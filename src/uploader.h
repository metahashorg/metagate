#ifndef UPLOADER_H
#define UPLOADER_H

#include <mutex>
#include <string>

#include <QString>
#include <QObject>

#include "client.h"

#include "VersionWrapper.h"

#include "TimerClass.h"

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

class Uploader : public TimerClass {
Q_OBJECT
private:

    struct Servers {
        std::string dev;
        std::string prod;
    };

public:

    using Callback = std::function<void()>;

public:

    explicit Uploader(auth::Auth &auth, MainWindow &mainWindow);

    ~Uploader();

signals:

    void checkedUpdatesHtmls(const TypedException &exception);

public:

    static std::pair<QString, QString> getLastVersion(const QString &pagesPath);

    static QString getVersionApp(const QString &pagesPath);

    static void setLastVersion(const QString &pagesPath, const QString &folder, const QString &version);

    static Servers getServers();

    static LastHtmlVersion getLastHtmlVersion();

signals:

    void callbackCall(Uploader::Callback callback);

public slots:

    void run();

    void onCallbackCall(Uploader::Callback callback);

    void uploadEvent();

    void onLogined(bool isInit, const QString login);

signals:

    void generateUpdateHtmlsEvent();

    void generateUpdateApp(const QString version, const QString reference, const QString message);

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
