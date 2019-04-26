#include "Paths.h"

#include <QStandardPaths>
#include <QApplication>
#include <QSettings>
#include <QMessageBox>

#include <mutex>

#include "utils.h"
#include "check.h"
#include "Log.h"

constexpr auto *metahashWalletPagesPathEnv = "METAHASH_WALLET_PAGES_PATH";

const static QString WALLET_PATH_DEFAULT = ".metahash_wallets/";

const static QString METAGATE_COMMON_PATH = ".metagate/";

const static QString LOG_PATH = "logs/";

const static QString PAGES_PATH = "pages/";

const static QString SETTINGS_NAME = "settings.ini";

const static QString SETTINGS_NAME_OLD = "settingsOld.ini";

const static QString STORAGE_NAME = "storage.ini";

const static QString MAC_ADDRESS_NAME = "mac.txt";

const static QString DB_PATH = "database/";

const static QString AUTOUPDATER_PATH = "autoupdater/";

const static QString NS_LOOKUP_PATH = "./";

static bool isInitializePagesPath = false;

static bool isInitializeSettingsPath = false;

QString getWalletPath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_PATH_DEFAULT);
    createFolder(res);
    return res;
}

QString getLogPath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH, LOG_PATH);
    createFolder(res);
    return res;
}

QString getDbPath() {
    removeFolder(makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH, "bd"));
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH, DB_PATH);
    createFolder(res);
    return res;
}

QString getNsLookupPath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH, NS_LOOKUP_PATH);
    createFolder(res);
    return res;
}

static QString getStartSettingsPath() {
    const auto path = qgetenv(metahashWalletPagesPathEnv);
    if (!path.isEmpty()) {
        return QString(path);
    }

    const QString path1(makePath(QApplication::applicationDirPath(), "startSettings/"));
    const QString path2(makePath(QApplication::applicationDirPath(), "../WalletMetahash/startSettings/"));
    const QString path3(makePath(QApplication::applicationDirPath(), "../../WalletMetahash/startSettings/"));
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

static void initializePagesPath() {
    CHECK(!isInitializePagesPath, "Already initialized page path");

    const QString startSettingsPath = getStartSettingsPath();
    const QString pagesPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH, PAGES_PATH);

    if (!isExistFolder(pagesPath)) {
        LOG << "Create pages folder: " << pagesPath << " " << startSettingsPath;
        CHECK(copyRecursively(startSettingsPath, pagesPath, true, false), "not copy pages");
        removeFile(makePath(pagesPath, SETTINGS_NAME));
    }

    isInitializePagesPath = true;
}

QString getPagesPath() {
    CHECK(isInitializePagesPath, "Not initialize page path");

    return makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH, PAGES_PATH);
}

static void initializeSettingsPath() {
    CHECK(!isInitializeSettingsPath, "Already initialized settings path");

    const QString startSettingsPath = getStartSettingsPath();
    const QString startSettings = makePath(startSettingsPath, SETTINGS_NAME);

    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH);
    createFolder(res);
    const QString settings = makePath(res, SETTINGS_NAME);
    const QString pagesPath = getPagesPath();

    const auto replaceSettings = [&] {
        bool isNotify = false;
        if (isExistFile(settings)) {
            const QString settingsOld = makePath(res, SETTINGS_NAME_OLD);
            copyFile(settings, settingsOld, true);

            const QSettings qsettings(settings, QSettings::IniFormat);
            if (qsettings.contains("notify") && qsettings.value("notify").toBool()) {
                isNotify = true;
            }
        }
        copyFile(startSettings, settings, true);

        if (isNotify) {
            QSettings qsettings(settings, QSettings::IniFormat);
            qsettings.setValue("notify", true);
            qsettings.sync();

            QMessageBox msgBox;
            msgBox.setText("Settings modified. See path " + settings);
            msgBox.exec();
        }
    };

    if (!isExistFile(settings)) {
        LOG << "Create settings: " << startSettings << " " << settings;
        replaceSettings();
    }

    const auto compareVersion = [&]() {// Чтобы в деструкторе закрылось
        const QSettings settingsFile(settings, QSettings::IniFormat);
        const QSettings startSettingsFile(startSettings, QSettings::IniFormat);
        return settingsFile.value("version") == startSettingsFile.value("version");
    };
    if (!compareVersion()) {
        LOG << "Replace settings: " << startSettings << " " << settings;
        replaceSettings();
    }

#ifdef VERSION_SETTINGS
    QSettings qsettings(settings, QSettings::IniFormat);
    CHECK(qsettings.value("version") == VERSION_SETTINGS, "Incorrect version settings");
#endif

    isInitializeSettingsPath = true;
}

QString getSettingsPath() {
    CHECK(isInitializeSettingsPath, "Not initialize settings path");

    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH);
    const QString settings = makePath(res, SETTINGS_NAME);
    return settings;
}

QString getStoragePath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH);
    createFolder(res);
    const QString storage = makePath(res, STORAGE_NAME);
    return storage;
}

QString getMacFilePath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH);
    createFolder(res);
    const QString storage = makePath(res, MAC_ADDRESS_NAME);
    return storage;
}

QString getAutoupdaterPath() {
    const QString path1(makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), METAGATE_COMMON_PATH, AUTOUPDATER_PATH));
    QDir dirTmp(path1);
    if (!dirTmp.exists()) {
        CHECK(dirTmp.mkpath(path1), "dont create autoupdater path");
    }
    return path1;
}

QString getTmpAutoupdaterPath() {
    return makePath(getAutoupdaterPath(), "folder/");
}

void clearAutoupdatersPath() {
    auto remove = [](const QString &dirPath) {
        QDir dir(dirPath);
        if (dir.exists()) {
            CHECK(dir.removeRecursively(), "dont remove autoupdater path");
            CHECK(dir.mkpath(dirPath), "dont create autoupdater path");
        }
    };

    remove(getAutoupdaterPath());
    remove(getTmpAutoupdaterPath());
}

void initializeAllPaths() {
    initializePagesPath();
    initializeSettingsPath();
}
