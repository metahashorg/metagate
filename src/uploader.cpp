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
#include <QDomElement>
#include <QApplication>
#include <QDebug>

#include "mainwindow.h"
#include "auth/Auth.h"

#include "check.h"
#include "duration.h"
#include "utilites/unzip.h"
#include "utilites/platform.h"
#include "utilites/VersionWrapper.h"
#include "Log.h"
#include "utilites/utils.h"
#include "utilites/machine_uid.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

#include "Paths.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("UPL");

std::mutex Uploader::lastVersionMut;

#ifdef Q_OS_WINDOWS
const QString Uploader::MAINTENANCETOOL = QStringLiteral("MGInstaller.exe");
#else
const QString Uploader::MAINTENANCETOOL = QStringLiteral("MGInstaller");
#endif

QString Uploader::getMaintenanceToolExe()
{
    QDir dir(qApp->applicationDirPath());
#ifdef Q_OS_MACOS
    dir.cdUp();
    dir.cdUp();
    dir.cdUp();
    dir.cd(MAINTENANCETOOL + QStringLiteral(".app"));
    dir.cd(QStringLiteral("Contents"));
    dir.cd(QStringLiteral("MacOS"));
#endif
    return dir.filePath(MAINTENANCETOOL);
}

// TODO change that
static QString m_repoUrl;
QString Uploader::getRepoUrl()
{
    return m_repoUrl;
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
    servers.prod = settings.value("servers/production").toString();
    CHECK(settings.contains("servers/development"), "settings development server not found");
    servers.dev = settings.value("servers/development").toString();

    return servers;
}

QString Uploader::getServerName()
{
    const Servers servers = Uploader::getServers();
    if (isProductionSetup) {
        LOG << "Set production server " << servers.prod;
        CHECK(!servers.prod.isEmpty(), "Empty server name");
        return servers.prod;
    } else {
        LOG << "Set development server " << servers.dev;
        CHECK(!servers.dev.isEmpty(), "Empty server name");
        return servers.dev;
    }
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
    serverName = getServerName();

    Q_CONNECT(this, &Uploader::generateUpdateHtmlsEvent, &mainWindow, &MainWindow::updateHtmlsEvent);
    Q_CONNECT(this, &Uploader::generateUpdateApp, &mainWindow, &MainWindow::updateAppEvent);

    Q_CONNECT(&auth, &auth::Auth::logined, this, &Uploader::onLogined);

    client.setParent(this);

    Q_CONNECT(this, &Uploader::callbackCall, this, &Uploader::onCallbackCall);
    Q_CONNECT(&client, &SimpleClient::callbackCall, this, &Uploader::callbackCall);

    Q_REG(Uploader::Callback, "Uploader::Callback");

    currentBeginPath = getPagesPath();
    const auto &lastVersionPair = Uploader::getLastVersion(currentBeginPath);
    currFolder = lastVersionPair.first;
    lastVersion = lastVersionPair.second;

    currentAppVersion = Version(VERSION_STRING);

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("timeouts_sec/uploader"), "settings timeouts not found");
    timeout = seconds(settings.value("timeouts_sec/uploader").toInt());

    client.moveToThread(TimerClass::getThread());

    emit auth.reEmit();

    moveToThread(TimerClass::getThread());
}

Uploader::~Uploader() {
    TimerClass::exit();
}

void Uploader::onCallbackCall(Uploader::Callback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void Uploader::startMethod() {
    uploadEvent();
}

void Uploader::timerMethod() {
    uploadEvent();
}

void Uploader::finishMethod() {
    // empty
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
    if (serverName == "") {
        return;
    }

    const auto callbackGetHtmls = [this](const SimpleClient::Response &response) {
        CHECK(!response.exception.isSet(), "Server error: " + response.exception.toString());
        const QJsonDocument document = QJsonDocument::fromJson(QString::fromStdString(response.response).toUtf8());
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

        const auto interfaceGetCallback = [this, version, hash, folderServer](const SimpleClient::Response &response) {
            versionHtmlForUpdate = "";
            CHECK(!response.exception.isSet(), "Server error: " + response.exception.toString());

            if (version == lastVersion && folderServer == currFolder) { // Так как это callback, то проверим еще раз
                return;
            }

            QCryptographicHash hashAlg(QCryptographicHash::Md5);
            hashAlg.addData(response.response.data(), response.response.size());
            const QString hashStr(hashAlg.result().toHex());
            CHECK(hashStr == hash, ("hash zip not equal response hash: hash zip: " + hashStr + ", hash response: " + hash + ", response size " + QString::number(response.response.size())).toStdString());

            removeOlderFolders(makePath(currentBeginPath, mainWindow.getCurrentHtmls().folderName), mainWindow.getCurrentHtmls().lastVersion);

            const QString archiveFilePath = makePath(currentBeginPath, version + ".zip");
            writeToFileBinary(archiveFilePath, response.response, false);

            const QString extractedPath = makePath(currentBeginPath, folderServer, version);
            extractDir(archiveFilePath, extractedPath);
            LOG << "Extracted " << extractedPath << "." << "Size: " << response.response.size();
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

    ////////////////
    LOG << "Start " << versionForUpdate;
    if (!versionForUpdate.isEmpty())
        return;


    const auto checkUpdates = [this](const QString &url) {
        LOG << url;
        QProcess checkPrc;
        const QStringList args{QStringLiteral("--checkupdates"),
            QStringLiteral("--addRepository"), url};
        LOG << Uploader::getMaintenanceToolExe();
        checkPrc.start(Uploader::getMaintenanceToolExe(), args);

        CHECK(checkPrc.waitForStarted(), std::string("Process waitForStarted error ") + std::to_string(checkPrc.error()));
        CHECK(checkPrc.waitForFinished(), std::string("Process waitForFinished error ") + std::to_string(checkPrc.error()));

        QByteArray errStr = checkPrc.readAllStandardError();
        LOG << "res " << errStr.toStdString();

        QByteArray result = checkPrc.readAll();
        LOG << "res " << result.toStdString();

        QDomDocument doc;
        doc.setContent(result);

        if (!doc.isNull() && doc.firstChildElement().hasChildNodes()) {
            QStringList updates;
            const QDomElement root = doc.firstChildElement(QLatin1String("updates"));
            QDomElement upd = root.firstChildElement(QLatin1String("update"));
            for (; !upd.isNull(); upd = upd.nextSiblingElement(QLatin1String("update"))) {
                const QString u = upd.attribute(QLatin1String("name"))
                    + QStringLiteral(" -> ")
                    + upd.attribute(QLatin1String("version"));
                updates.append(u);
            }
            versionForUpdate = QStringLiteral("updated");
            emit generateUpdateApp(updates.join(QStringLiteral(" ")), QString::number(updates.count()), "");
        }
    };

    const auto callbackRepo = [checkUpdates](const SimpleClient::Response &response) {
        CHECK(!response.exception.isSet(), "Server error: " + response.exception.toString());

        LOG << response.response;

        const QJsonDocument document = QJsonDocument::fromJson(QString::fromStdString(response.response).toUtf8());
        const QJsonObject root = document.object();
        CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
        const auto &dataJson = root.value("data").toObject();
        CHECK(dataJson.contains("url") && dataJson.value("url").isString(), "url field not found");
        const QString url = dataJson.value("url").toString();
        m_repoUrl = url;
        checkUpdates(url);
    };

    client.sendMessagePost(
        QUrl(serverName),
        QString::fromStdString("{\"id\": \"" + std::to_string(id) +
           "\",\"version\":\"1.0.0\",\"method\":\"app.repo\", \"token\":\"" + apiToken.toStdString() +
           "\", \"uid\": \"" + getMachineUid() + "\", \"params\":[{\"platform\": \"" + osName.toStdString() + "\"}]}"),
        callbackRepo, timeout
        );
    id++;
}
