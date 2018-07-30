#ifndef UPLOADER_H
#define UPLOADER_H

#include <mutex>
#include <string>

#include <QString>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "client.h"

#include "WindowEvents.h"

#include "VersionWrapper.h"

class MainWindow;

class ServerName {
public:

    void setServerName(const QString &serverName) {
        std::lock_guard<std::mutex> lock(mut);
        updateServerName = serverName;
    }

    QString getServerName() const {
        std::lock_guard<std::mutex> lock(mut);
        return updateServerName;
    }

private:

    QString updateServerName = "";

    mutable std::mutex mut;

};

struct LastHtmlVersion {
    QString htmlsRootPath;

    QString folderName;

    QString lastVersion;
};

class Uploader : public QObject {
Q_OBJECT

private:

    struct Servers {
        std::string dev;
        std::string prod;
    };

public:

    Uploader(MainWindow *mainWindow, ServerName &serverName);

    ~Uploader() override;

public:

    static std::pair<QString, QString> getLastVersion(const QString &pagesPath);

    static QString getVersionApp(const QString &pagesPath);

    static void setLastVersion(const QString &pagesPath, const QString &folder, const QString &version);

    static Servers getServers();

    static LastHtmlVersion getLastHtmlVersion();

public:

    void start();

public slots:

    void run();

    void callbackCall(ReturnCallback callback);

    void timerEvent();

signals:

    void finished();

    void generateEvent(WindowEvent event);

    void generateUpdateApp(const QString version, const QString reference, const QString message);

private:

    MainWindow *mainWindow;

    ServerName &serverName;

    int id = 1;

    SimpleClient client;

    QString currentBeginPath;

    QString currFolder;

    QString lastVersion;

    Version currentAppVersion;

    QString versionForUpdate = "";

    QString versionHtmlForUpdate = "";

    QThread thread1;

    QTimer qtimer;

private:

    static std::mutex lastVersionMut;

};

#endif // UPLOADER_H
