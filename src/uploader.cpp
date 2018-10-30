#include "uploader.h"

#include <thread>

#include <QThread>
#include <QTimer>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QCryptographicHash>
#include <QSettings>

#include "mainwindow.h"

#include "check.h"
#include "duration.h"
#include "unzip.h"
#include "platform.h"
#include "VersionWrapper.h"
#include "Log.h"
#include "utils.h"
#include "SlotWrapper.h"
#include "Paths.h"

std::mutex Uploader::lastVersionMut;

static QString toHash(const QString &valueQ) {
    const std::string value = valueQ.toStdString();
    QByteArray array(value.data(), value.size());
    return QString(QCryptographicHash::hash((array),QCryptographicHash::Md5).toHex());
}

std::pair<QString, QString> Uploader::getLastVersion(const QString &pagesPath) {
    std::lock_guard<std::mutex> lock(lastVersionMut);
    const QString filePath = makePath(pagesPath, "lastVersion.txt");
    QFile inputFile(filePath);
    CHECK(inputFile.open(QIODevice::ReadOnly), "Not open file " + filePath.toStdString());
    QTextStream in(&inputFile);
    QString lastVersion = in.readLine();
    CHECK(!lastVersion.isNull(), "Incorrect lastVersion file");
    QString folderName = in.readLine();
    CHECK(!folderName.isNull(), "Incorrect lastVersion file");
    return std::make_pair(folderName, lastVersion);
}

void Uploader::setLastVersion(const QString &pagesPath, const QString &folderName, const QString &version) {
    std::lock_guard<std::mutex> lock(lastVersionMut);
    const QString filePath = makePath(pagesPath, "lastVersion.txt");
    writeToFile(filePath, (version + "\n" + folderName + "\n").toStdString(), false);
}

LastHtmlVersion Uploader::getLastHtmlVersion() {
    LastHtmlVersion result;
    result.htmlsRootPath = getPagesPath();
    const auto &last = getLastVersion(result.htmlsRootPath);
    result.folderName = last.first;
    result.lastVersion = last.second;
    result.fullPath = makePath(result.htmlsRootPath, result.folderName, result.lastVersion);
    return result;
}

Uploader::Servers Uploader::getServers() {
    Servers servers;
    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("servers/production"), "production server not found");
    servers.prod = settings.value("servers/production").toString().toStdString();
    CHECK(settings.contains("servers/development"), "development server not found");
    servers.dev = settings.value("servers/development").toString().toStdString();

    return servers;
}

Uploader::Uploader(MainWindow *mainWindow)
    : mainWindow(mainWindow)
{
    CHECK(mainWindow != nullptr, "maiWindow == nullptr");

    const Servers servers = getServers();
    if (isProductionSetup) {
        LOG << "Set production server " << servers.prod;
        CHECK(!servers.prod.empty(), "Empty server name");
        serverName = QString::fromStdString(servers.prod);
    } else {
        LOG << "Set development server " << servers.dev;
        CHECK(!servers.dev.empty(), "Empty server name");
        serverName = QString::fromStdString(servers.dev);
    }

    CHECK(connect(this, SIGNAL(generateEvent(WindowEvent)), mainWindow, SLOT(processEvent(WindowEvent))), "not connect");
    CHECK(connect(this, SIGNAL(generateUpdateApp(QString,QString,QString)), mainWindow, SLOT(updateAppEvent(QString,QString,QString))), "not connect");

    client.setParent(this);

    CHECK(connect(&client, &SimpleClient::callbackCall, this, &Uploader::callbackCall), "not connect");

    currentBeginPath = getPagesPath();
    const auto &lastVersionPair = Uploader::getLastVersion(currentBeginPath);
    currFolder = lastVersionPair.first;
    lastVersion = lastVersionPair.second;

    currentAppVersion = Version(VERSION_STRING);

    CHECK(QObject::connect(&thread1,SIGNAL(started()),this,SLOT(run())), "not connect");
    CHECK(QObject::connect(this,SIGNAL(finished()),&thread1,SLOT(terminate())), "not connect");

    const milliseconds msTimer = 10s;
    qtimer.moveToThread(&thread1);
    qtimer.setInterval(msTimer.count());
    CHECK(connect(&qtimer, SIGNAL(timeout()), this, SLOT(uploadEvent())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(started()), SLOT(start())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(finished()), SLOT(stop())), "not connect");

    client.moveToThread(&thread1);

    moveToThread(&thread1);
}

Uploader::~Uploader() {
    thread1.quit();
    if (!thread1.wait(3000)) {
        thread1.terminate();
        thread1.wait();
    }
}

void Uploader::start() {
    thread1.start();
}

void Uploader::callbackCall(SimpleClient::ReturnCallback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void Uploader::run() {
    emit uploadEvent();
}

static void clearFolderHtmls(const QString &folderHtmls, const QString &currentVersion) {
    QDir sourceDir(folderHtmls);
    const auto mask = QDir::Dirs | QDir::NoDotAndDotDot;
    for (const QString &dirName: sourceDir.entryList(mask)) {
        if (!isPathEquals(dirName, currentVersion)) {
            QDir d(makePath(folderHtmls, dirName));
            d.removeRecursively();
        }
    }
}

void Uploader::uploadEvent() {
BEGIN_SLOT_WRAPPER
    const QString UPDATE_API = serverName;

    if (UPDATE_API == "") {
        return;
    }

    auto callbackGetHtmls = [this, UPDATE_API](const std::string &result, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.description + ". " + exception.content);
        const QJsonDocument document = QJsonDocument::fromJson(QString::fromStdString(result).toUtf8());
        const QJsonObject root = document.object();
        CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
        const auto &dataJson = root.value("data").toObject();
        CHECK(dataJson.contains("version") && dataJson.value("version").isString(), "version field not found");
        const QString version = dataJson.value("version").toString();
        CHECK(dataJson.contains("hash") && dataJson.value("hash").isString(), "hash field not found");
        const QString hash = dataJson.value("hash").toString();
        CHECK(dataJson.contains("url") && dataJson.value("url").isString(), "url field not found");
        const QString url = dataJson.value("url").toString();

        LOG << "Server html version " << version << " " << hash << " " << url.toStdString().substr(0, url.toStdString().find("?secure")) << ". Current version " << lastVersion;

        const QString folderServer = toHash(UPDATE_API);
        if (hash == "false") {
            return;
        }
        if ((version == lastVersion && folderServer == currFolder) || version == versionHtmlForUpdate) {
            return;
        }

        auto interfaceGetCallback = [this, version, hash, UPDATE_API, folderServer](const std::string &result, const SimpleClient::ServerException &exception) {
            versionHtmlForUpdate = "";
            CHECK(!exception.isSet(), "Server error: " + exception.description + ". " + exception.content);

            if (version == lastVersion && folderServer == currFolder) { // Так как это callback, то проверим еще раз
                return;
            }

            QCryptographicHash hashAlg(QCryptographicHash::Md5);
            hashAlg.addData(result.data(), result.size());
            const QString hashStr(hashAlg.result().toHex());
            CHECK(hashStr == hash, ("hash zip not equal response hash: hash zip: " + hashStr + ", hash response: " + hash).toStdString());

            clearFolderHtmls(makePath(currentBeginPath, folderServer), lastVersion);

            const QString archiveFilePath = makePath(currentBeginPath, version + ".zip");
            writeToFileBinary(archiveFilePath, result, false);

            const QString extractedPath = makePath(currentBeginPath, folderServer, version);
            extractDir(archiveFilePath, extractedPath);
            LOG << "Extracted " << extractedPath << ".";
            removeFile(archiveFilePath);

            Uploader::setLastVersion(currentBeginPath, folderServer, version);

            lastVersion = version;
            currFolder = folderServer;

            emit generateEvent(WindowEvent::RELOAD_PAGE);
        };

        LOG << "download html";
        versionHtmlForUpdate = version;
        client.sendMessageGet(url, interfaceGetCallback);
        id++;
    };

    client.sendMessagePost(QUrl(UPDATE_API), QString::fromStdString("{\"id\": \"" + std::to_string(id) + "\",\"version\":\"1.0.0\",\"method\":\"interface.get.url\", \"token\":\"\", \"params\":[]}"), callbackGetHtmls);
    id++;

    auto callbackAppVersion = [this, UPDATE_API](const std::string &result, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.description + ". " + exception.content);

        const QJsonDocument document = QJsonDocument::fromJson(QString::fromStdString(result).toUtf8());
        const QJsonObject root = document.object();
        CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
        const auto &dataJson = root.value("data").toObject();
        CHECK(dataJson.contains("version") && dataJson.value("version").isString(), "version field not found");
        const QString version = dataJson.value("version").toString();
        CHECK(dataJson.contains("reference") && dataJson.value("reference").isString(), "reference field not found");
        const QString reference = dataJson.value("reference").toString();
        CHECK(dataJson.contains("autoupdate_reference") && dataJson.value("autoupdate_reference").isString(), "autoupdate_reference field not found");
        const QString autoupdater = dataJson.value("autoupdate_reference").toString();

        const Version nextVersion(version.toStdString());

        LOG << "New app version " << nextVersion.makeStr() << " " << reference << " " << autoupdater.toStdString().substr(0, autoupdater.toStdString().find("?secure")) << ". Current app version " << currentAppVersion.makeStr();

        if (reference == "false") {
            return;
        }
        if (nextVersion <= currentAppVersion || version == versionForUpdate) {
            return;
        }

        auto autoupdateGetCallback = [this, version, reference](const std::string &result, const SimpleClient::ServerException &exception) {
            versionForUpdate.clear();
            LOG << "autoupdater callback";
            CHECK(!exception.isSet(), "Server error: " + exception.description + ". " + exception.content);

            clearAutoupdatersPath();
            const QString autoupdaterPath = getAutoupdaterPath();
            const QString archiveFilePath = makePath(autoupdaterPath, version + ".zip");
            writeToFileBinary(archiveFilePath, result, false);

            extractDir(archiveFilePath, getTmpAutoupdaterPath());
            LOG << "Extracted autoupdater " << getTmpAutoupdaterPath();

            emit generateUpdateApp(version, reference, "");
        };

        LOG << "New app version download";
        client.sendMessageGet(QUrl(autoupdater), autoupdateGetCallback);

        versionForUpdate = version;
    };

    client.sendMessagePost(QUrl(UPDATE_API), QString::fromStdString("{\"id\": \"" + std::to_string(id) + "\",\"version\":\"1.0.0\",\"method\":\"app.version\", \"token\":\"\", \"params\":[{\"platform\": \"" + osName.toStdString() + "\"}]}"), callbackAppVersion);
    id++;
END_SLOT_WRAPPER
}
