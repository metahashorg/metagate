#include "uploader.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QCryptographicHash>
#include <QSettings>
#include <QTextStream>
#include <QUrl>
#include <QProcess>
#include <QDomDocument>
#include <QApplication>
#include <QDebug>

#include "mainwindow.h"
#include "auth/Auth.h"

#include "check.h"
#include "duration.h"
#include "unzip.h"
#include "platform.h"
#include "VersionWrapper.h"
#include "Log.h"
#include "utils.h"
#include "SlotWrapper.h"
#include "Paths.h"
#include "QRegister.h"
#include "machine_uid.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("UPL");

std::mutex Uploader::lastVersionMut;

#ifdef Q_OS_WINDOWS
const QString Uploader::MAINTENANCETOOL = QStringLiteral("maintenancetool.exe");
#elif Q_OS_MACOS
const QString Uploader::MAINTENANCETOOL = QStringLiteral("maintenancetool");
#else
const QString Uploader::MAINTENANCETOOL = QStringLiteral("maintenancetool");
#endif

QString Uploader::getMaintenanceToolExe()
{
    QDir dir(qApp->applicationDirPath());
#ifdef Q_OS_MACOS
    dir.cdUp();
    dir.cdUp();
#endif
    return dir.filePath(MAINTENANCETOOL);
}

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
    CHECK(settings.contains("servers/production"), "settings production server not found");
    servers.prod = settings.value("servers/production").toString().toStdString();
    CHECK(settings.contains("servers/development"), "settings development server not found");
    servers.dev = settings.value("servers/development").toString().toStdString();

    return servers;
}

static milliseconds getTimerInterval() {
    QSettings settings(getSettingsPath(), QSettings::IniFormat);

    CHECK(settings.contains("downloader/period_sec"), "downloader/period_sec not found");
    const seconds minMsTimer = 15s;
    const milliseconds msTimer = std::max(minMsTimer, seconds(settings.value("downloader/period_sec").toInt()));
    return msTimer;
}

Uploader::Uploader(auth::Auth &auth, MainWindow &mainWindow)
    : TimerClass(getTimerInterval(), nullptr)
    , auth(auth)
    , mainWindow(mainWindow)
{
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

    CHECK(connect(this, &Uploader::timerEvent, this, &Uploader::uploadEvent), "not connect onTimerEvent");
    CHECK(connect(this, &Uploader::startedEvent, this, &Uploader::run), "not connect run");

    CHECK(connect(this, &Uploader::generateUpdateHtmlsEvent, &mainWindow, &MainWindow::updateHtmlsEvent), "not connect processEvent");
    CHECK(connect(this, &Uploader::generateUpdateApp, &mainWindow, &MainWindow::updateAppEvent), "not connect updateAppEvent");

    CHECK(connect(&auth, &auth::Auth::logined, this, &Uploader::onLogined), "not connect onLogined");

    client.setParent(this);

    CHECK(connect(this, &Uploader::callbackCall, this, &Uploader::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(&client, &SimpleClient::callbackCall, this, &Uploader::callbackCall), "not connect callbackCall");

    Q_REG(Uploader::Callback, "Uploader::Callback");

    currentBeginPath = getPagesPath();
    const auto &lastVersionPair = Uploader::getLastVersion(currentBeginPath);
    currFolder = lastVersionPair.first;
    lastVersion = lastVersionPair.second;

    currentAppVersion = Version(VERSION_STRING);

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("timeouts_sec/uploader"), "settings timeouts not found");
    timeout = seconds(settings.value("timeouts_sec/uploader").toInt());

    client.moveToThread(&thread1);

    emit auth.reEmit();

    moveToThread(&thread1);
}

Uploader::~Uploader() {
    TimerClass::exit();
}

void Uploader::onCallbackCall(Uploader::Callback callback) {
    BEGIN_SLOT_WRAPPER
            callback();
    END_SLOT_WRAPPER
}

void Uploader::run() {
    BEGIN_SLOT_WRAPPER
            emit uploadEvent();
    END_SLOT_WRAPPER
}

static void removeOlderFolders(const QString &folderHtmls, const QString &currentVersion) {
    QDir sourceDir(folderHtmls);
    const auto mask = QDir::Dirs | QDir::NoDotAndDotDot;
    for (const QString &dirName: sourceDir.entryList(mask)) {
        if (!isPathEquals(dirName, currentVersion)) {
            const QString pathRemove = makePath(folderHtmls, dirName);
            LOG << "Remove older folder " << pathRemove;
            QDir d(pathRemove);
            d.removeRecursively();
        }
    }
}

void Uploader::onLogined(bool isInit, const QString login) {
    BEGIN_SLOT_WRAPPER
            if (isInit && !login.isEmpty()) {
        emit auth.getLoginInfo(auth::Auth::LoginInfoCallback([this](const auth::LoginInfo &info){
            apiToken = info.token;
            emit uploadEvent();
        }, [](const TypedException &e) {
            LOG << "Error: " << e.description;
        }, std::bind(&Uploader::callbackCall, this, _1)));
    }
    END_SLOT_WRAPPER
}

void Uploader::uploadEvent() {
    BEGIN_SLOT_WRAPPER
            if (serverName == "") {
        return;
    }

    const auto callbackGetHtmls = [this](const std::string &result, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
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

        LOG << PeriodicLog::make("n_htm") << "Server html version " << version << " " << hash << " " << url.toStdString().substr(0, url.toStdString().find("?secure")) << ". Current version " << lastVersion;

        const QString folderServer = toHash(serverName);
        if (hash == "false") {
            emit checkedUpdatesHtmls(TypedException());
            return;
        }
        if (version == lastVersion && folderServer == currFolder) {
            emit checkedUpdatesHtmls(TypedException());
            return;
        }
        if (version == versionHtmlForUpdate) {
            return;
        }

        const auto interfaceGetCallback = [this, version, hash, folderServer](const std::string &result, const SimpleClient::ServerException &exception) {
            versionHtmlForUpdate = "";
            CHECK(!exception.isSet(), "Server error: " + exception.toString());

            if (version == lastVersion && folderServer == currFolder) { // Так как это callback, то проверим еще раз
                return;
            }

            QCryptographicHash hashAlg(QCryptographicHash::Md5);
            hashAlg.addData(result.data(), result.size());
            const QString hashStr(hashAlg.result().toHex());
            CHECK(hashStr == hash, ("hash zip not equal response hash: hash zip: " + hashStr + ", hash response: " + hash + ", response size " + QString::number(result.size())).toStdString());

            removeOlderFolders(makePath(currentBeginPath, mainWindow.getCurrentHtmls().folderName), mainWindow.getCurrentHtmls().lastVersion);

            const QString archiveFilePath = makePath(currentBeginPath, version + ".zip");
            writeToFileBinary(archiveFilePath, result, false);

            const QString extractedPath = makePath(currentBeginPath, folderServer, version);
            extractDir(archiveFilePath, extractedPath);
            LOG << "Extracted " << extractedPath << "." << "Size: " << result.size();
            removeFile(archiveFilePath);

            Uploader::setLastVersion(currentBeginPath, folderServer, version);

            lastVersion = version;
            currFolder = folderServer;

            emit generateUpdateHtmlsEvent();

            emit checkedUpdatesHtmls(TypedException());
        };

        LOG << "download html";
        countDownloads["html_" + version]++;
        CHECK(countDownloads["html_" + version] < 3, "Maximum download");
        versionHtmlForUpdate = version;
        client.sendMessageGet(url, interfaceGetCallback); // Без таймаута, так как загрузка большого бинарника
        id++;
    };
    client.sendMessagePost(
                QUrl(serverName),
                QString::fromStdString("{\"id\": \"" + std::to_string(id) +
                                       "\",\"version\":\"1.0.0\",\"method\":\"interface.get.url\", \"token\":\"" + apiToken.toStdString() +
                                       "\", \"uid\": \"" + getMachineUid() + "\", \"params\":[]}")
                , callbackGetHtmls, timeout
                );
    id++;

    QProcess checkPrc;
    checkPrc.start(Uploader::getMaintenanceToolExe(), QStringList() << "--checkupdates");
    if (!checkPrc.waitForStarted())
        return;

    if (!checkPrc.waitForFinished())
        return;

    QByteArray result = checkPrc.readAll();
    //qDebug() << "res " << result;

    QDomDocument document;
    document.setContent(result);

    if (!document.isNull() && document.firstChildElement().hasChildNodes()) {
        emit generateUpdateApp(QStringLiteral(""), QStringLiteral(""), "");
    }
    /*
    const auto callbackAppVersion = [this](const std::string &result, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());

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

        LOG << PeriodicLog::make("n_app") << "New app version " << nextVersion.makeStr() << " " << reference << " " << autoupdater.toStdString().substr(0, autoupdater.toStdString().find("?secure")) << ". Current app version " << currentAppVersion.makeStr();

        if (reference == "false") {
            return;
        }
        if (nextVersion <= currentAppVersion || version == versionForUpdate) {
            return;
        }

        const auto autoupdateGetCallback = [this, version, reference](const std::string &result, const SimpleClient::ServerException &exception) {
            versionForUpdate.clear();
            LOG << "autoupdater callback";
            CHECK(!exception.isSet(), "Server error: " + exception.toString());

            clearAutoupdatersPath();
            const QString autoupdaterPath = getAutoupdaterPath();
            const QString archiveFilePath = makePath(autoupdaterPath, version + ".zip");
            writeToFileBinary(archiveFilePath, result, false);

            extractDir(archiveFilePath, getTmpAutoupdaterPath());
            LOG << "Extracted autoupdater " << getTmpAutoupdaterPath();

            emit generateUpdateApp(version, reference, "");
            versionForUpdate = version;
        };

        LOG << "New app version download";
        countDownloads["p_" + version]++;
        CHECK(countDownloads["p_" + version] < 3, "Maximum download");
        client.sendMessageGet(QUrl(autoupdater), autoupdateGetCallback); // Без таймаута, так как загрузка большого бинарника

        versionForUpdate = version;
    };

    client.sendMessagePost(
        QUrl(serverName),
        QString::fromStdString("{\"id\": \"" + std::to_string(id) +
        "\",\"version\":\"1.0.0\",\"method\":\"app.version\", \"token\":\"" + apiToken.toStdString() +
        "\", \"uid\": \"" + getMachineUid() + "\", \"params\":[{\"platform\": \"" + osName.toStdString() + "\"}]}"),
        callbackAppVersion, timeout
    );
    */
    id++;
    END_SLOT_WRAPPER
}
