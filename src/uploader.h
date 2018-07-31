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

    explicit Uploader(MainWindow *mainWindow);

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

    QString serverName;

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
