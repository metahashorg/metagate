#ifndef UPLOADER_H
#define UPLOADER_H

#include <mutex>
#include <string>

#include <QString>
#include <QObject>
#include <QThread>
#include <QTimer>

#include "client.h"

#include "VersionWrapper.h"

class MainWindow;
struct TypedException;

struct LastHtmlVersion {
    QString htmlsRootPath;

    QString folderName;

    QString lastVersion;

    QString fullPath;
};

class Uploader : public QObject {
Q_OBJECT

private:

    struct Servers {
        std::string dev;
        std::string prod;
    };

public:

    explicit Uploader(MainWindow &mainWindow);

    ~Uploader() override;

signals:

    void checkedUpdatesHtmls(const TypedException &exception);

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

    void callbackCall(SimpleClient::ReturnCallback callback);

    void uploadEvent();

signals:

    void finished();

    void generateUpdateHtmlsEvent();

    void generateUpdateApp(const QString version, const QString reference, const QString message);

private:

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

    QThread thread1;

    QTimer qtimer;

    seconds timeout;

private:

    static std::mutex lastVersionMut;

};

#endif // UPLOADER_H
