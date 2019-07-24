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

namespace transactions {
class Transactions;
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

    using SignMessageCallback = CallbackWrapper<void(const QString &signature, const QString &pubkey)>;

    using SignMessage2Callback = CallbackWrapper<void(const QString &signature, const QString &pubkey, const QString &tx)>;

    using GettedNonceCallback = CallbackWrapper<void(size_t nonce)>;

    using SignAndSendMessageCallback = CallbackWrapper<void(bool success)>;

    using GetPrivateKeyCallback = CallbackWrapper<void(const QString &result)>;

    using SavePrivateKeyCallback = CallbackWrapper<void(bool success)>;

    using SaveRawPrivateKeyCallback = CallbackWrapper<void(const QString &address)>;

    using GetRawPrivateKeyCallback = CallbackWrapper<void(const QString &result)>;

public:

    explicit Wallets(auth::Auth &auth, QObject *parent = nullptr);

    ~Wallets() override;

    void setTransactions(transactions::Transactions *txs);

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

    void signMessage(bool isMhc, const QString &address, const QString &text, const QString &password, const wallets::Wallets::SignMessageCallback &callback);

    void signMessage2(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const SignMessage2Callback &callback);

    void signAndSendMessage(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const QString &paramsJson, const SignAndSendMessageCallback &callback);

    void signAndSendMessageDelegate(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &valueDelegate, const QString &nonce, bool isDelegate, const QString &paramsJson, const SignAndSendMessageCallback &callback);

    void getOnePrivateKey(bool isMhc, const QString &address, bool isCompact, const GetPrivateKeyCallback &callback);

    void savePrivateKey(bool isMhc, const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback);

    void saveRawPrivateKey(bool isMhc, const QString &rawPrivateKey, const QString &password, const SaveRawPrivateKeyCallback &callback);

    void getRawPrivateKey(bool isMhc, const QString &address, const QString &password, const GetRawPrivateKeyCallback &callback);

private slots:

    void onCreateWallet(bool isMhc, const QString &password, const CreateWalletCallback &callback);

    void onCreateWatchWallet(bool isMhc, const QString &address, const CreateWatchWalletCallback &callback);

    void onRemoveWatchWallet(bool isMhc, const QString &address, const RemoveWatchWalletCallback &callback);

    void onCheckWalletExist(bool isMhc, const QString &address, const CheckWalletExistCallback &callback);

    void onCheckWalletPassword(bool isMhc, const QString &address, const QString &password, const CheckWalletPasswordCallback &callback);

    void onCheckAddress(const QString &address, const CheckAddressCallback &callback);

    void onCreateContractAddress(const QString &address, int nonce, const CreateContractAddressCallback &callback);

    void onSignMessage(bool isMhc, const QString &address, const QString &text, const QString &password, const wallets::Wallets::SignMessageCallback &callback);

    void onSignMessage2(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const SignMessage2Callback &callback);

    void onSignAndSendMessage(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const QString &paramsJson, const SignAndSendMessageCallback &callback);

    void onSignAndSendMessageDelegate(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &valueDelegate, const QString &nonce, bool isDelegate, const QString &paramsJson, const SignAndSendMessageCallback &callback);

    void onGetOnePrivateKey(bool isMhc, const QString &address, bool isCompact, const GetPrivateKeyCallback &callback);

    void onSavePrivateKey(bool isMhc, const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback);

    void onSaveRawPrivateKey(bool isMhc, const QString &rawPrivateKey, const QString &password, const SaveRawPrivateKeyCallback &callback);

    void onGetRawPrivateKey(bool isMhc, const QString &address, const QString &password, const GetRawPrivateKeyCallback &callback);

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

    void findNonceAndProcessWithTxManager(bool isMhc, const QString &address, const QString &nonce, const QString &paramsJson, const GettedNonceCallback &callback);

private:

    QString walletPath;

    QString userName;

    QString token;

    std::vector<FolderWalletInfo> folderWalletsInfos;

    QFileSystemWatcher fileSystemWatcher;

    EventWatcher eventWatcher;

    transactions::Transactions *txs = nullptr;

};

} // namespace wallets

#endif // WALLETS_WALLETS_H