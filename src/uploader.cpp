#include "uploader.h"

#include <iostream>
#include <thread>

#include <QThread>
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QApplication>
#include <QTimer>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
//#include <QByteArray>
#include <QCryptographicHash>

#include "mainwindow.h"

#include "check.h"
#include "duration.h"
#include "unzip.h"
#include "platform.h"
#include "VersionWrapper.h"
#include "Log.h"
#include "utils.h"
#include "stringUtils.h"

constexpr auto * metahashWalletPagesPathEnv = "METAHASH_WALLET_PAGES_PATH";

std::mutex Uploader::lastVersionMut;

static QString toHash(const QString &valueQ) {
    const std::string value = valueQ.toStdString();
    QByteArray array(value.data(), value.size());
    return QString(QCryptographicHash::hash((array),QCryptographicHash::Md5).toHex());
}

std::pair<QString, QString> Uploader::getLastVersion(const QString &pagesPath) {
    std::lock_guard<std::mutex> lock(lastVersionMut);
    const QString filePath = QDir(pagesPath).filePath("lastVersion.txt");
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
    const QString filePath = QDir(pagesPath).filePath("lastVersion.txt");
    writeToFile(filePath, (version + "\n" + folderName + "\n").toStdString(), false);
}

QString Uploader::getPagesPath() {
    const auto path = qgetenv(metahashWalletPagesPathEnv);
    if (!path.isEmpty())
        return QString(path);

    const QString path1(QDir(QApplication::applicationDirPath()).filePath("pages/"));
    const QString path2(QDir(QApplication::applicationDirPath()).filePath("../WalletMetahash/pages/"));
    const QString path3(QDir(QApplication::applicationDirPath()).filePath("../../WalletMetahash/pages/"));
    QString currentBeginPath;
    QDir dirTmp;
    if (dirTmp.exists(path1)) {
        currentBeginPath = path1;
    } else if (dirTmp.exists(path2)){
        currentBeginPath = path2;
    } else {
        currentBeginPath = path3;
    }

    return currentBeginPath;
}

LastHtmlVersion Uploader::getLastHtmlVersion() {
    LastHtmlVersion result;
    result.htmlsRootPath = getPagesPath();
    const auto &last = getLastVersion(result.htmlsRootPath);
    result.folderName = last.first;
    result.lastVersion = last.second;
    return result;
}

QString Uploader::getAutoupdaterPath() {
    const QString path1(QDir(QApplication::applicationDirPath()).filePath("autoupdater/"));
    QDir dirTmp(path1);
    if (!dirTmp.exists()) {
        CHECK(dirTmp.mkpath(path1), "dont create autoupdater path");
    }
    return path1;
}

QString Uploader::getTmpAutoupdaterPath() {
    return QDir(getAutoupdaterPath()).filePath("folder/");
}

static std::pair<bool, std::string> parseServer(const std::string &str) {
    const std::string trimStr = trim(str);
    CHECK(!trimStr.empty(), "Incorrect str: empty");
    const size_t foundSpace = trimStr.find(" ");
    CHECK(foundSpace != trimStr.npos, "Not found dot in string " + str);
    const std::string type = trim(trimStr.substr(0, foundSpace));
    const std::string server = trim(trimStr.substr(foundSpace + 1));
    CHECK(!server.empty(), "Empty server");
    if (type == "development") {
        return std::make_pair(false, server);
    } else if (type == "production") {
        return std::make_pair(true, server);
    } else {
        throwErr("Incorrect type " + str);
    }
}

Uploader::Servers Uploader::getServers() {
    const QString currentBeginPath = QDir(getPagesPath()).filePath("servers.txt");

    Servers servers;
    QFile inputFile(currentBeginPath);
    CHECK(inputFile.open(QIODevice::ReadOnly), "Not open file " + currentBeginPath.toStdString());
    QTextStream in(&inputFile);
    QString firstServer = in.readLine();
    CHECK(!firstServer.isNull(), "Incorrect servers file");
    QString secondServer = in.readLine();
    CHECK(!secondServer.isNull(), "Incorrect servers file");
    const auto pairServer1 = parseServer(firstServer.toStdString());
    const auto pairServer2 = parseServer(secondServer.toStdString());
    if (pairServer1.first) {
        servers.prod = pairServer1.second;
    } else {
        servers.dev = pairServer1.second;
    }
    if (pairServer2.first) {
        servers.prod = pairServer2.second;
    } else {
        servers.dev = pairServer2.second;
    }
    return servers;
}

Uploader::Uploader(MainWindow *mainWindow, ServerName &serverName)
    : mainWindow(mainWindow)
    , serverName(serverName)
{
    CHECK(mainWindow != nullptr, "maiWindow == nullptr");

    const Servers servers = getServers();
    if (isProductionSetup) {
        LOG << "Set production server " << servers.prod;
        CHECK(!servers.prod.empty(), "Empty server name");
        serverName.setServerName(QString::fromStdString(servers.prod));
    } else {
        LOG << "Set development server " << servers.dev;
        CHECK(!servers.dev.empty(), "Empty server name");
        serverName.setServerName(QString::fromStdString(servers.dev));
    }

    CHECK(connect(this, SIGNAL(generateEvent(WindowEvent)), mainWindow, SLOT(processEvent(WindowEvent))), "not connect");
    CHECK(connect(this, SIGNAL(generateUpdateApp(QString,QString,QString)), mainWindow, SLOT(updateAppEvent(QString,QString,QString))), "not connect");

    client.setParent(this);

    CHECK(connect(&client, SIGNAL(callbackCall(ReturnCallback)), this, SLOT(callbackCall(ReturnCallback))), "not connect");

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
    CHECK(connect(&qtimer, SIGNAL(timeout()), this, SLOT(timerEvent())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(started()), SLOT(start())), "not connect");
    CHECK(qtimer.connect(&thread1, SIGNAL(finished()), SLOT(stop())), "not connect");

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

void Uploader::callbackCall(ReturnCallback callback) {
    try {
        callback();
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (...) {
        LOG << "Unknown error";
    }
}

void Uploader::run() {
    emit timerEvent();
}

void Uploader::timerEvent() {
    try {
        const QString UPDATE_API = serverName.getServerName();

        if (UPDATE_API == "") {
            return;
        }

        auto callbackGetHtmls = [this, UPDATE_API](const std::string &result) {
            CHECK(result != SimpleClient::ERROR_BAD_REQUEST, "incorrect result");
            const QJsonDocument document = QJsonDocument::fromJson(QString::fromStdString(result).toUtf8());
            const QJsonObject root = document.object();
            CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
            const auto &dataJson = root.value("data").toObject();
            CHECK(dataJson.contains("version") && dataJson.value("version").isString(), "version field not found");
            const QString version = dataJson.value("version").toString();
            CHECK(dataJson.contains("hash") && dataJson.value("hash").isString(), "hash field not found");
            const QString hash = dataJson.value("hash").toString();

            LOG << "Server html version " << version << " " << hash << ". Current version " << lastVersion;

            const QString folderServer = toHash(UPDATE_API);

            if (hash == "false") {
                return;
            }
            if (version == lastVersion && folderServer == currFolder) {
                return;
            }

            auto interfaceGetCallback = [this, version, hash, UPDATE_API, folderServer](const std::string &result) {
                CHECK(result != SimpleClient::ERROR_BAD_REQUEST, "Bad request");

                if (version == lastVersion && folderServer == currFolder) { // Так как это callback, то проверим еще раз
                    return;
                }

                QCryptographicHash hashAlg(QCryptographicHash::Md5);
                hashAlg.addData(result.data(), result.size());
                const QString hashStr(hashAlg.result().toHex());
                if (hashStr != hash) {
                    LOG << "hashStr != hash " << hashStr << " " << hash;
                    return;
                }

                const QString archiveFilePath = QDir(currentBeginPath).filePath(version + ".zip");
                writeToFileBinary(archiveFilePath, result, false);

                const QString extractedPath = QDir(QDir(currentBeginPath).filePath(folderServer)).filePath(version);
                extractDir(archiveFilePath, extractedPath);
                LOG << "Extracted " << extractedPath << ".";

                Uploader::setLastVersion(currentBeginPath, folderServer, version);

                lastVersion = version;
                currFolder = folderServer;

                emit generateEvent(WindowEvent::RELOAD_PAGE);
            };
            client.sendMessagePost(QUrl(UPDATE_API), QString::fromStdString("{\"id\": \"" + std::to_string(id) + "\",\"version\":\"1.0.0\",\"method\":\"interface.get\", \"token\":\"\", \"params\":[]}"), interfaceGetCallback);
            id++;
        };

        client.sendMessagePost(QUrl(UPDATE_API), QString::fromStdString("{\"id\": \"" + std::to_string(id) + "\",\"version\":\"1.0.0\",\"method\":\"interface\", \"token\":\"\", \"params\":[]}"), callbackGetHtmls);
        id++;

        auto callbackAppVersion = [this, UPDATE_API](const std::string &result) {
            CHECK(result != SimpleClient::ERROR_BAD_REQUEST, "Incorrect result");

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

            auto autoupdateGetCallback = [this, nextVersion, version, reference](const std::string &result) {
                LOG << "autoupdater callback";
                CHECK(result != SimpleClient::ERROR_BAD_REQUEST, "Incorrect result");

                const QString autoupdaterPath = getAutoupdaterPath();
                const QString archiveFilePath = QDir(autoupdaterPath).filePath(version + ".zip");
                writeToFileBinary(archiveFilePath, result, false);

                extractDir(archiveFilePath, getTmpAutoupdaterPath());
                LOG << "Extracted autoupdater " << getTmpAutoupdaterPath();

                emit generateUpdateApp(version, reference, "");
            };

            client.sendMessageGet(QUrl(autoupdater), autoupdateGetCallback);

            versionForUpdate = version;
        };

        client.sendMessagePost(QUrl(UPDATE_API), QString::fromStdString("{\"id\": \"" + std::to_string(id) + "\",\"version\":\"1.0.0\",\"method\":\"app.version\", \"token\":\"\", \"params\":[{\"platform\": \"" + osName.toStdString() + "\"}]}"), callbackAppVersion);
        id++;
    } catch (const Exception &e) {
        LOG << "Error " << e;
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
    } catch (...) {
        LOG << "Unknown error";
    }
}
