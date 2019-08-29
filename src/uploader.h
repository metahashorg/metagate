#ifndef UPLOADER_H
#define UPLOADER_H

#include <mutex>
#include <string>

#include <QString>
#include <QObject>

#include "Network/SimpleClient.h"

#include "utilites/VersionWrapper.h"

#include "qt_utilites/TimerClass.h"

class MainWindow;
struct TypedException;

namespace auth {
class Auth;
}

struct LastHtmlVersion {
    QString htmlsRootPath;

    QString folderName;

    QString lastVersion;

    QString fullPath;
};

class Uploader : public QObject, public TimerClass {
Q_OBJECT
private:

    struct Servers {
        QString dev;
        QString prod;
    };

public:
    const static QString MAINTENANCETOOL;

    static QString getMaintenanceToolExe();
    static QString getRepoUrl();

public:

    using Callback = std::function<void()>;

public:

    explicit Uploader(auth::Auth &auth, MainWindow &mainWindow);

    ~Uploader() override;

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

signals:

    void checkedUpdatesHtmls(const TypedException &exception);

public:

    static std::pair<QString, QString> getLastVersion(const QString &pagesPath);

    static QString getVersionApp(const QString &pagesPath);

    static void setLastVersion(const QString &pagesPath, const QString &folder, const QString &version);

    static Servers getServers();

    static QString getServerName();

    static LastHtmlVersion getLastHtmlVersion();

signals:

    void callbackCall(Uploader::Callback callback);

public slots:

    void onCallbackCall(Uploader::Callback callback);

    void onLogined(bool isInit, const QString login);

signals:

    void generateUpdateHtmlsEvent();

    void generateUpdateApp(const QString version, const QString reference, const QString message);

private:

    void uploadEvent();

private:

    auth::Auth &auth;

    MainWindow &mainWindow;

    QString serverName;

    int id = 1;

    SimpleClient client;

    QString currentBeginPath;

    QString currFolder;

    QString lastVersion;

    Version currentAppVersion;

    QString versionForUpdate = "";

    QString versionHtmlForUpdate = "";

    seconds timeout;

    std::map<QString, int> countDownloads;

    QString apiToken;

private:

    static std::mutex lastVersionMut;

};

#endif // UPLOADER_H
