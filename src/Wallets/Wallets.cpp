#include "Wallets.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "Log.h"
#include "check.h"

#include "utilites/utils.h"

#include "Wallet.h"
#include "BtcWallet.h"
#include "EthWallet.h"
#include "WalletRsa.h"

#include "Paths.h"

#include "auth/Auth.h"

SET_LOG_NAMESPACE("WLTS");

namespace wallets {

const QString Wallets::defaultUsername = "_unregistered";

Wallets::Wallets(auth::Auth &auth, QObject *parent)
    : TimerClass(5s, parent)
    , walletDefaultPath(getWalletPath())
{
    Q_CONNECT(&auth, &auth::Auth::logined2, this, &Wallets::onLogined);
    Q_CONNECT(&fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &Wallets::onDirChanged);

    Q_CONNECT(this, &Wallets::getListWallets, this, &Wallets::onGetListWallets);

    Q_REG(WalletsListCallback, "WalletsListCallback");
    Q_REG(Wallets::WalletCurrency, "Wallets::WalletCurrency");

    fileSystemWatcher.moveToThread(TimerClass::getThread());
    moveToThread(TimerClass::getThread()); // TODO вызывать в TimerClass
}

Wallets::~Wallets() {
    TimerClass::exit();
}

void Wallets::onGetListWallets(const WalletCurrency &type, const WalletsListCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        CHECK(!walletPath.isEmpty(), "Wallet path not set");
        if (type == WalletCurrency::Tmh) {
            return std::make_tuple(userName, Wallet::getAllWalletsInfoInFolder(walletPath, false));
        } else if (type == WalletCurrency::Mth) {
            return std::make_tuple(userName, Wallet::getAllWalletsInfoInFolder(walletPath, true));
        } else if (type == WalletCurrency::Btc) {
            const std::vector<std::pair<QString, QString>> res = BtcWallet::getAllWalletsInFolder(walletPath);
            std::vector<WalletInfo> result;
            result.reserve(res.size());
            std::transform(res.begin(), res.end(), std::back_inserter(result), [](const auto &pair) {
                return WalletInfo(pair.first, pair.second, WalletInfo::Type::Key);
            });
            return std::make_tuple(userName, result);
        } else if (type == WalletCurrency::Eth) {
            const std::vector<std::pair<QString, QString>> res = EthWallet::getAllWalletsInFolder(walletPath);
            std::vector<WalletInfo> result;
            result.reserve(res.size());
            std::transform(res.begin(), res.end(), std::back_inserter(result), [](const auto &pair) {
                return WalletInfo(pair.first, pair.second, WalletInfo::Type::Key);
            });
            return std::make_tuple(userName, result);
        } else {
            throwErr("Incorrect type");
        }
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::startMethod() {

}

void Wallets::timerMethod() {

}

void Wallets::finishMethod() {

}

void Wallets::setPathsImpl(QString newPatch, QString newUserName) {
    userName = newUserName;

    if (walletPath == newPatch) {
        return;
    }

    walletPath = newPatch;
    CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
    createFolder(walletPath);

    for (const FolderWalletInfo &folderInfo: folderWalletsInfos) {
        fileSystemWatcher.removePath(folderInfo.walletPath.absolutePath());
    }
    folderWalletsInfos.clear();

    const auto setPathToWallet = [this](const QString &suffix, const QString &name) {
        const QString curPath = makePath(walletPath, suffix);
        createFolder(curPath);
        folderWalletsInfos.emplace_back(curPath, name);
        fileSystemWatcher.addPath(curPath);
    };

    setPathToWallet(EthWallet::subfolder(), "eth");
    setPathToWallet(BtcWallet::subfolder(), "btc");
    setPathToWallet(Wallet::chooseSubfolder(true), "mhc");
    setPathToWallet(Wallet::chooseSubfolder(false), "tmh");

    LOG << "Wallets path " << walletPath;
}

void Wallets::onLogined(bool /*isInit*/, const QString &login, const QString &token_) {
BEGIN_SLOT_WRAPPER
    if (!login.isEmpty()) {
        setPathsImpl(makePath(walletDefaultPath, login), login);
        token = token_;
    } else {
        setPathsImpl(makePath(walletDefaultPath, defaultUsername), defaultUsername);
    }
END_SLOT_WRAPPER
}

void Wallets::onDirChanged(const QString &dir) {
BEGIN_SLOT_WRAPPER
END_SLOT_WRAPPER
}

} // namespace wallets
