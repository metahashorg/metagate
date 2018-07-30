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
#include "Paths.h"

#include <iostream>

#ifdef TARGET_WINDOWS

#include <shlobj.h>
#include <shlwapi.h>
#include <objbase.h>

static const QString pathToUpdater = "updater";
static const QString pathToNewApplication = "{A}";
static const QString pathToNewApplication2 = "{A2}";

void updateAndRestart() {
    const QString thisName = "MetaGate.exe";

    const QString autoupdateFolder = getTmpAutoupdaterPath();

    const QString thisPath = QCoreApplication::applicationDirPath();

    const QString tmpFilePath = makePath(thisPath, "tmp.txt");
    QFile::remove(tmpFilePath);
    QFile file(tmpFilePath);
    const bool isUac = !file.open(QFile::WriteOnly);

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
    const QString newAppPath = QDir(tmpPath).filePath(!isUac ? pathToNewApplication : pathToNewApplication2);
    QDir(newAppPath).removeRecursively();
    CHECK(QDir(newAppPath).mkpath(newAppPath), ("dont create new app folder " + newAppPath).toStdString());
    const QString zipAppPath = QDir(newAppPath).filePath(zipName);

    QFile::remove(updaterPath);
    QFile::remove(zipAppPath);

    CHECK(QFile::copy(updaterNameOld, updaterPath), "dont copy updater ");
    CHECK(QFile::copy(zipNameOld, zipAppPath), "dont copy zip");

    copyRecursively(sevenZipFolderOld, newAppPath);

    CHECK(QFile::remove(updaterNameOld), "dont remove updater");
    CHECK(QFile::remove(zipNameOld), "dont remove zip");
    QDir(autoupdateFolder).removeRecursively();


    LOG << "paths for restart " << tmpPath << " " << thisPath;
    LOG << "New app path " << newAppPath;

    if (!isUac) {
        QProcess process;
        std::cout << "Tt " << ("\"" + updaterPath + "\" \"" + newAppPath + "\" \"" + zipAppPath + "\" " + thisName + " \"" + thisPath + "\"").toStdString() << std::endl;
        CHECK(process.startDetached("\"" + updaterPath + "\" \"" + newAppPath + "\" \"" + zipAppPath + "\" " + thisName + " \"" + thisPath + "\""), "dont start updater process");
    } else {
        LOG << "UAC mode";

        SHELLEXECUTEINFO shExInfo = {0};
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
        shExInfo.hwnd = 0;
        auto tt1 = QString("runas").toStdWString();
        shExInfo.lpVerb = tt1.c_str();                // Operation to perform
        auto tt2 = QString("powershell.exe").toStdWString();
        shExInfo.lpFile = tt2.c_str();       // Application to start
        auto tt3 = QString("-WindowStyle Hidden -Command \"" + updaterPath + "\" \\\"" + newAppPath + "\\\" \\\"" + zipAppPath + "\\\" " + thisName + " \\\"" + thisPath + "\\\"").toStdWString();
        LOG << QString("-WindowStyle Hidden -Command \"" + updaterPath + "\" \\\"" + newAppPath + "\\\" \\\"" + zipAppPath + "\\\" " + thisName + " \\\"" + thisPath + "\\\"");
        shExInfo.lpParameters = tt3.c_str();                  // Additional parameters
        shExInfo.lpDirectory = 0;
        shExInfo.nShow = SW_SHOW;
        shExInfo.hInstApp = 0;

        CHECK(ShellExecuteEx(&shExInfo), "Not execute");
    }
    QApplication::exit(SIMPLE_EXIT);
}
#else

static const QString pathToUpdater = "updater";
static const QString pathToNewApplication = "WalletMetahashUpdater";

void updateAndRestart() {
    const QString autoupdateFolder = getTmpAutoupdaterPath();

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
