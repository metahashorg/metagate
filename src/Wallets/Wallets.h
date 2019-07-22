#ifndef WALLETS_WALLETS_H
#define WALLETS_WALLETS_H

#include "qt_utilites/TimerClass.h"

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "WalletInfo.h"

#include <QDir>
#include <QFileSystemWatcher>

#include <vector>

#include "qt_utilites/EventWatcher.h"

namespace auth {
class Auth;
}

namespace wallets {

class Wallets: public ManagerWrapper, public TimerClass {
    Q_OBJECT
public:

    using WalletsListCallback = CallbackWrapper<void(const QString &userName, const std::vector<WalletInfo> &walletAddresses)>;

    using CreateWatchsCallback = CallbackWrapper<void(const std::vector<std::pair<QString, QString>> &created)>;

    using CreateWalletCallback = CallbackWrapper<void(const QString &fullPath, const std::string &pubkey, const std::string &address, const std::string &exampleMessage, const std::string &sign)>;

    using CreateWatchWalletCallback = CallbackWrapper<void(const QString &fullPath)>;

    using RemoveWatchWalletCallback = CallbackWrapper<void()>;

    using CheckWalletExistCallback = CallbackWrapper<void(bool isExist)>;

    using CheckWalletPasswordCallback = CallbackWrapper<void(bool success)>;

    using CheckAddressCallback = CallbackWrapper<void(bool success)>;

    using CreateContractAddressCallback = CallbackWrapper<void(const QString &address)>;

public:

    explicit Wallets(auth::Auth &auth, QObject *parent = nullptr);

    ~Wallets() override;

public:

    const QString walletDefaultPath;

    const static QString defaultUsername;

signals:

    void usernameChanged(const QString &newUserName);

    void watchWalletsAdded(bool isMhc, const std::vector<std::pair<QString, QString>> &created);

    void mhcWalletCreated(bool isMhc, const QString &address);

    void mhcWatchWalletCreated(bool isMhc, const QString &address);

    void mhcWatchWalletRemoved(bool isMhc, const QString &address);

signals:

    void getListWallets(const wallets::WalletCurrency &type, const WalletsListCallback &callback);

    void getListWallets2(const wallets::WalletCurrency &type, const QString &expectedUsername, const WalletsListCallback &callback);

    void createWatchWalletsList(bool isMhc, const std::vector<QString> &addresses, const CreateWatchsCallback &callback);

private slots:

    void onGetListWallets(const wallets::WalletCurrency &type, const WalletsListCallback &callback);

    void onGetListWallets2(const wallets::WalletCurrency &type, const QString &expectedUsername, const WalletsListCallback &callback);

    void onCreateWatchWalletsList(bool isMhc, const std::vector<QString> &addresses, const CreateWatchsCallback &callback);

///////////
/// MHC ///
///////////

signals:

    void createWallet(bool isMhc, const QString &password, const CreateWalletCallback &callback);

    void createWatchWallet(bool isMhc, const QString &address, const CreateWatchWalletCallback &callback);

    void removeWatchWallet(bool isMhc, const QString &address, const RemoveWatchWalletCallback &callback);

    void checkWalletExist(bool isMhc, const QString &address, const CheckWalletExistCallback &callback);

    void checkWalletPassword(bool isMhc, const QString &address, const QString &password, const CheckWalletPasswordCallback &callback);

    void checkAddress(const QString &address, const CheckAddressCallback &callback);

    void createContractAddress(const QString &address, int nonce, const CreateContractAddressCallback &callback);

private slots:

    void onCreateWallet(bool isMhc, const QString &password, const CreateWalletCallback &callback);

    void onCreateWatchWallet(bool isMhc, const QString &address, const CreateWatchWalletCallback &callback);

    void onRemoveWatchWallet(bool isMhc, const QString &address, const RemoveWatchWalletCallback &callback);

    void onCheckWalletExist(bool isMhc, const QString &address, const CheckWalletExistCallback &callback);

    void onCheckWalletPassword(bool isMhc, const QString &address, const QString &password, const CheckWalletPasswordCallback &callback);

    void onCheckAddress(const QString &address, const CheckAddressCallback &callback);

    void onCreateContractAddress(const QString &address, int nonce, const CreateContractAddressCallback &callback);

private slots:

    void onLogined(bool isInit, const QString &login, const QString &token);

    void onDirChanged(const QString &dir);

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

private:

    struct FolderWalletInfo {
        QDir walletPath;
        QString nameWallet;

        FolderWalletInfo(const QDir &walletPath, const QString &nameWallet)
            : walletPath(walletPath)
            , nameWallet(nameWallet)
        {}
    };

private:

    void setPathsImpl(QString newPatch, QString newUserName);

private:

    QString walletPath;

    QString userName;

    QString token;

    std::vector<FolderWalletInfo> folderWalletsInfos;

    QFileSystemWatcher fileSystemWatcher;

    EventWatcher eventWatcher;

};

} // namespace wallets

#endif // WALLETS_WALLETS_H
