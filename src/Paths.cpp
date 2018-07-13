#include "Paths.h"

#include <QStandardPaths>

#include "utils.h"

const static QString WALLET_PATH_DEFAULT = ".metahash_wallets/";

const static QString WALLET_COMMON_PATH = ".metagate/";

const static QString LOG_PATH = "logs/";

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

QString getNsLookupPath() {
    const QString res = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_COMMON_PATH, NS_LOOKUP_PATH);
    createFolder(res);
    return res;
}
