#include "Paths.h"

#include <QStandardPaths>
#include <QApplication>

#include <mutex>

#include "utils.h"
#include "check.h"
#include "Log.h"

constexpr auto *metahashWalletPagesPathEnv = "METAHASH_WALLET_PAGES_PATH";

const static QString WALLET_PATH_DEFAULT = ".metahash_wallets/";

const static QString WALLET_COMMON_PATH = ".metagate/";

const static QString LOG_PATH = "logs/";

const static QString PAGES_PATH = "pages/";

const static QString BD_PATH = "bd/";

const static QString AUTOUPDATER_PATH = "autoupdater/";

const static QString NS_LOOKUP_PATH = "./";

QString getWalletPath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_PATH_DEFAULT);
    createFolder(res);
    return res;
}

QString getLogPath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_COMMON_PATH, LOG_PATH);
    createFolder(res);
    return res;
}

QString getBdPath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_COMMON_PATH, BD_PATH);
    createFolder(res);
    return res;
}

QString getNsLookupPath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_COMMON_PATH, NS_LOOKUP_PATH);
    createFolder(res);
    return res;
}

static QString getOldPagesPath() {
    const auto path = qgetenv(metahashWalletPagesPathEnv);
    if (!path.isEmpty())
        return QString(path);

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

QString getPagesPath() {
    static std::mutex mut;

    const QString oldPagesPath = getOldPagesPath();

    const QString newPagesPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_COMMON_PATH, PAGES_PATH);

    std::lock_guard<std::mutex> lock(mut);
    if (!isExistFolder(newPagesPath)) {
        LOG << "Create pages folder: " << newPagesPath << " " << oldPagesPath;
        CHECK(copyRecursively(oldPagesPath, newPagesPath), "not copy pages");
    }

    return newPagesPath;
}

QString getSettingsPath() {
    return getPagesPath();
}

QString getAutoupdaterPath() {
    const QString path1(makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_COMMON_PATH, AUTOUPDATER_PATH));
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
