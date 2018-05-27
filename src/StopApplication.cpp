#include "StopApplication.h"

#include <QStandardPaths>
#include <QApplication>
#include <QProcess>
#include <QFile>
#include <QDir>

#include "uploader.h"

#include "utils.h"
#include "check.h"
#include "Log.h"

#include <iostream>

#ifdef TARGET_WINDOWS

static const QString pathToUpdater = "updater";
static const QString pathToNewApplication = "{A}";

void updateAndRestart() {
    const QString thisName = "MetaGate.exe";

    const QString autoupdateFolder = Uploader::getTmpAutoupdaterPath();

    const QString updaterName = "unpack.bat";
    const QString zipName = "release.zip";
    const QString sevenZipFolder = "7z";

    const QString updaterNameOld = QDir(autoupdateFolder).filePath(updaterName);
    const QString zipNameOld = QDir(autoupdateFolder).filePath(zipName);
    const QString sevenZipFolderOld = QDir(autoupdateFolder).filePath(sevenZipFolder);

    CHECK(QFile(updaterNameOld).exists(), "updater path not exist");
    CHECK(QFile(zipNameOld).exists(), "newApp path not exist");
    CHECK(QDir(sevenZipFolderOld).exists(), "sevenZipFolder path not exist");

    const QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString updaterFolder = QDir(tmpPath).filePath(pathToUpdater);
    CHECK(QDir(updaterFolder).mkpath(updaterFolder), "dont create updater folder");
    const QString updaterPath = QDir(updaterFolder).filePath(updaterName);
    const QString newAppPath = QDir(tmpPath).filePath(pathToNewApplication);
    QDir(newAppPath).removeRecursively();
    CHECK(QDir(newAppPath).mkpath(newAppPath), "dont create new app folder");
    const QString zipAppPath = QDir(newAppPath).filePath(zipName);

    QFile::remove(updaterPath);
    QFile::remove(zipAppPath);

    CHECK(QFile::copy(updaterNameOld, updaterPath), "dont copy updater ");
    CHECK(QFile::copy(zipNameOld, zipAppPath), "dont copy zip");

    copyRecursively(sevenZipFolderOld, newAppPath);

    CHECK(QFile::remove(updaterNameOld), "dont remove updater");
    CHECK(QFile::remove(zipNameOld), "dont remove zip");
    QDir(autoupdateFolder).removeRecursively();



    const QString thisPath = QCoreApplication::applicationDirPath();
    LOG << "paths for restart " << tmpPath << " " << thisPath;
    LOG << "New app path " << newAppPath;

    QProcess process;
    std::cout << "Tt " << ("\"" + updaterPath + "\" \"" + newAppPath + "\" \"" + zipAppPath + "\" " + thisName + " \"" + thisPath + "\"").toStdString() << std::endl;
    CHECK(process.startDetached("\"" + updaterPath + "\" \"" + newAppPath + "\" \"" + zipAppPath + "\" " + thisName + " \"" + thisPath + "\""), "dont start updater process");
    //process.waitForFinished();
    //process.close();
    QApplication::exit(SIMPLE_EXIT);
}
#else

static const QString pathToUpdater = "updater";
static const QString pathToNewApplication = "WalletMetahashUpdater";

void updateAndRestart() {
    const QString autoupdateFolder = Uploader::getTmpAutoupdaterPath();

    const QString updaterName = "updater";
    const QString xmlName = "file_list.xml";
    const QString zipName = "app.zip";

    const QString updaterNameOld = QDir(autoupdateFolder).filePath(updaterName);
    const QString xmlNameOld = QDir(autoupdateFolder).filePath(xmlName);
    const QString zipNameOld = QDir(autoupdateFolder).filePath(zipName);

    CHECK(QFile(updaterNameOld).exists(), "updater path not exist");
    CHECK(QFile(xmlNameOld).exists(), "scriptPatch path not exist");
    CHECK(QFile(zipNameOld).exists(), "newApp path not exist");

    const QString tmpPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation);
    const QString updaterFolder = QDir(tmpPath).filePath(pathToUpdater);
    CHECK(QDir(updaterFolder).mkpath(updaterFolder), "dont create updater folder");
    const QString updaterPath = QDir(updaterFolder).filePath(updaterName);
    const QString newAppPath = QDir(tmpPath).filePath(pathToNewApplication);
    CHECK(QDir(newAppPath).mkpath(newAppPath), "dont create newAppPath folder");
    const QString zipAppPath = QDir(newAppPath).filePath(zipName);
    const QString scriptPatch = QDir(newAppPath).filePath(xmlName);

    QFile::remove(updaterPath);
    QFile::remove(scriptPatch);
    QFile::remove(zipAppPath);

    CHECK(QFile::copy(updaterNameOld, updaterPath), "dont copy updater ");
    CHECK(QFile::copy(xmlNameOld, scriptPatch), "dont copy xml " + xmlNameOld.toStdString() + " " + scriptPatch.toStdString());
    CHECK(QFile::copy(zipNameOld, zipAppPath), "dont copy zip");

    CHECK(QFile::remove(updaterNameOld), "dont remove updater");
    CHECK(QFile::remove(xmlNameOld), "dont remove xml");
    CHECK(QFile::remove(zipNameOld), "dont remove zip");

    QString thisPath = QCoreApplication::applicationDirPath();
#ifdef TARGET_OS_MAC
    if (thisPath.contains("/Contents/MacOS")) {
        thisPath = QDir(thisPath).filePath("../..");
    }
#endif
    LOG << "paths for restart " << tmpPath << " \"" << thisPath << "\"";
    LOG << "Updater path " << updaterPath;
    LOG << "New app path " << newAppPath;

    CHECK(QFile(updaterPath).exists(), "updater path not exist");
    CHECK(QDir(newAppPath).exists(), "newAppPath path not exist");
    CHECK(QFile(scriptPatch).exists(), "scriptPatch path not exist");

    QProcess process;
    CHECK(process.startDetached(updaterPath + " --install-dir \"" + thisPath + "\" --package-dir " + newAppPath + " --script " + scriptPatch), "dont start updater process");
    QApplication::exit(SIMPLE_EXIT);
}
#endif
