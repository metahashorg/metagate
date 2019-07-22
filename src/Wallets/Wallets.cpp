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

#include "GetActualWalletsEvent.h"

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
    Q_CONNECT(this, &Wallets::getListWallets2, this, &Wallets::onGetListWallets2);
    Q_CONNECT(this, &Wallets::createWatchWalletsList, this, &Wallets::onCreateWatchWalletsList);

    Q_REG(WalletsListCallback, "WalletsListCallback");
    Q_REG(wallets::WalletCurrency, "wallets::WalletCurrency");
    Q_REG2(std::vector<QString>, "std::vector<QString>", false);
    Q_REG(CreateWatchCallback, "CreateWatchCallback");

    emit auth.reEmit();

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

void Wallets::onGetListWallets2(const wallets::WalletCurrency &type, const QString &expectedUsername, const WalletsListCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        if (expectedUsername == userName) {
            emit getListWallets(type, callback);
        } else {
            std::unique_ptr<GetActualWalletsEvent> event = std::make_unique<GetActualWalletsEvent>(TimerClass::getThread(), *this, expectedUsername, type, callback);

            Q_CONNECT(this, &Wallets::usernameChanged, event.get(), &GetActualWalletsEvent::changedUserName);

            eventWatcher.addEvent("getListWallet", std::move(event), 3s);
        }
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::onCreateWatchWalletsList(bool isMhc, const std::vector<QString> &addresses, const CreateWatchCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        std::vector<std::pair<QString, QString>> created;
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        for (const QString &addr : addresses) {
            if (Wallet::isWalletExists(walletPath, isMhc, addr.toStdString())) {
                continue;
            }
            Wallet::createWalletWatch(walletPath, isMhc, addr.toStdString());
            Wallet wallet(walletPath, isMhc, addr.toStdString());
            const QString walletFullPath = wallet.getFullPath();
            created.emplace_back(std::make_pair(addr, walletFullPath));
        }

        emit watchWalletsAdded(isMhc, created);

        return created;
    }, callback);
END_SLOT_WRAPPER
}

void Wallets::startMethod() {

}

void Wallets::timerMethod() {
    eventWatcher.checkEvents();
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

    emit usernameChanged(userName);
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
