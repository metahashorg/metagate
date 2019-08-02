#ifndef WALLETS_WALLETS_H
#define WALLETS_WALLETS_H

#include "qt_utilites/TimerClass.h"

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "WalletInfo.h"

#include <QDir>
#include <QFileSystemWatcher>

#include <vector>
#include <set>

#include "qt_utilites/EventWatcher.h"

namespace auth {
class Auth;
}

namespace transactions {
class Transactions;
struct SendParameters;
}

namespace utils {
class Utils;
}

struct BtcInput;

namespace wallets {

class Wallets: public ManagerWrapper, public TimerClass {
    Q_OBJECT
public:

    using WalletsList = std::vector<std::pair<QString, QString>>;

    using WalletsListCallback = CallbackWrapper<void(const QString &userName, const std::vector<WalletInfo> &walletAddresses)>;

    using CreateWatchsCallback = CallbackWrapper<void(const WalletsList &created)>;

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

    using CreateRsaKeyCallback = CallbackWrapper<void(const QString &pubkey)>;

    using GetRsaPublicKeyCallback = CallbackWrapper<void(const QString &pubkey)>;

    using CopyRsaKeyCallback = CallbackWrapper<void(bool success)>;

    using CreateEthKeyCallback = CallbackWrapper<void(const QString &address, const QString &fullPath)>;

    using SignMessageEthCallback = CallbackWrapper<void(const QString &result)>;

    using CreateBtcKeyCallback = CallbackWrapper<void(const QString &address, const QString &fullPath)>;

    using SignMessageBtcCallback = CallbackWrapper<void(const QString &result, const QString &hash, const std::set<std::string> &usedUtxos)>;

    using GetWalletFoldersCallback = CallbackWrapper<void(const QString &defaultFolder, const QString &folder, const QString &userName)>;

    using BackupKeysCallback = CallbackWrapper<void(const QString &fileName)>;

    using RestoreKeysCallback = CallbackWrapper<void(const QString &fileName)>;

    using ImportKeysCallback = CallbackWrapper<void(int count)>;

    using CalkKeysCallback = CallbackWrapper<void(const std::vector<QString> &addresses)>;

public:

    explicit Wallets(auth::Auth &auth, utils::Utils &utils, QObject *parent = nullptr);

    ~Wallets() override;

    void setTransactions(transactions::Transactions *txs);

public:

    const QString walletDefaultPath;

signals:

    void usernameChanged(const QString &newUserName);

    void watchWalletsAdded(bool isMhc, const WalletsList &created, const QString &username);

    void mhcWalletCreated(bool isMhc, const QString &address, const QString &username);

    void mhcWatchWalletCreated(bool isMhc, const QString &address, const QString &username);

    void mhcWatchWalletRemoved(bool isMhc, const QString &address, const QString &username);

    void dirChanged(const QString &absolutePath, const QString &nameCurrency);

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

    void createWatchWalletsList(bool isMhc, const std::vector<QString> &addresses, const CreateWatchsCallback &callback);

    void importKeys(bool isMhc, const QString &path, const ImportKeysCallback &callback);

    void calkKeys(bool isMhc, const QString &path, const CalkKeysCallback &callback);

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

    void onCreateWatchWalletsList(bool isMhc, const std::vector<QString> &addresses, const CreateWatchsCallback &callback);

    void onImportKeys(bool isMhc, const QString &path, const ImportKeysCallback &callback);

    void onCalkKeys(bool isMhc, const QString &path, const CalkKeysCallback &callback);

///////////
/// RSA ///
///////////

signals:

    void createRsaKey(bool isMhc, const QString &address, const QString &password, const CreateRsaKeyCallback &callback);

    void getRsaPublicKey(bool isMhc, const QString &address, const GetRsaPublicKeyCallback &callback);

    void copyRsaKey(bool isMhc, const QString &address, const QString &pathPub, const QString &pathPriv, const CopyRsaKeyCallback &callback);

    void copyRsaKeyToFolder(bool isMhc, const QString &address, const QString &path, const CopyRsaKeyCallback &callback);

private slots:

    void onCreateRsaKey(bool isMhc, const QString &address, const QString &password, const CreateRsaKeyCallback &callback);

    void onGetRsaPublicKey(bool isMhc, const QString &address, const GetRsaPublicKeyCallback &callback);

    void onCopyRsaKey(bool isMhc, const QString &address, const QString &pathPub, const QString &pathPriv, const CopyRsaKeyCallback &callback);

    void onCopyRsaKeyToFolder(bool isMhc, const QString &address, const QString &path, const CopyRsaKeyCallback &callback);

////////////////
/// ETHEREUM ///
////////////////

signals:

    void createEthKey(const QString &password, const CreateEthKeyCallback &callback);

    void signMessageEth(const QString &address, const QString &password, const QString &nonce, const QString &gasPrice, const QString &gasLimit, const QString &to, const QString &value, const QString &data, const SignMessageEthCallback &callback);

    void checkAddressEth(const QString &address, const CheckAddressCallback &callback);

    void savePrivateKeyEth(const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback);

    void getOnePrivateKeyEth(const QString &address, const GetPrivateKeyCallback &callback);

    void importKeysEth(const QString &path, const ImportKeysCallback &callback);

    void calkKeysEth(const QString &path, const CalkKeysCallback &callback);

private slots:

    void onCreateEthKey(const QString &password, const CreateEthKeyCallback &callback);

    void onSignMessageEth(const QString &address, const QString &password, const QString &nonce, const QString &gasPrice, const QString &gasLimit, const QString &to, const QString &value, const QString &data, const SignMessageEthCallback &callback);

    void onCheckAddressEth(const QString &address, const CheckAddressCallback &callback);

    void onSavePrivateKeyEth(const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback);

    void onGetOnePrivateKeyEth(const QString &address, const GetPrivateKeyCallback &callback);

    void onImportKeysEth(const QString &path, const ImportKeysCallback &callback);

    void onCalkKeysEth(const QString &path, const CalkKeysCallback &callback);

///////////
/// BTC ///
///////////

signals:

    void createBtcKey(const QString &password, const CreateBtcKeyCallback &callback);

    void checkAddressBtc(const QString &address, const CheckAddressCallback &callback);

    void signMessageBtcUsedUtxos(const QString &address, const QString &password, const std::vector<BtcInput> &inputs, const QString &toAddress, const QString &value, const QString &estimateComissionInSatoshi, const QString &fees, const std::set<std::string> &usedUtxos, const SignMessageBtcCallback &callback);

    void savePrivateKeyBtc(const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback);

    void getOnePrivateKeyBtc(const QString &address, const GetPrivateKeyCallback &callback);

    void importKeysBtc(const QString &path, const ImportKeysCallback &callback);

    void calkKeysBtc(const QString &path, const CalkKeysCallback &callback);

private slots:

    void onCreateBtcKey(const QString &password, const CreateBtcKeyCallback &callback);

    void onCheckAddressBtc(const QString &address, const CheckAddressCallback &callback);

    void onSignMessageBtcUsedUtxos(const QString &address, const QString &password, const std::vector<BtcInput> &inputs, const QString &toAddress, const QString &value, const QString &estimateComissionInSatoshi, const QString &fees, const std::set<std::string> &usedUtxos, const SignMessageBtcCallback &callback);

    void onSavePrivateKeyBtc(const QString &privateKey, const QString &password, const SavePrivateKeyCallback &callback);

    void onGetOnePrivateKeyBtc(const QString &address, const GetPrivateKeyCallback &callback);

    void onImportKeysBtc(const QString &path, const ImportKeysCallback &callback);

    void onCalkKeysBtc(const QString &path, const CalkKeysCallback &callback);

//////////////
/// COMMON ///
//////////////

signals:

    void getListWallets(const wallets::WalletCurrency &type, const WalletsListCallback &callback);

    void getListWallets2(const wallets::WalletCurrency &type, const QString &expectedUsername, const WalletsListCallback &callback);

    void getWalletFolders(const GetWalletFoldersCallback &callback);

    void backupKeys(const QString &caption, const BackupKeysCallback &callback);

    void restoreKeys(const QString &caption, const RestoreKeysCallback &callback);

    void openWalletPathInStandartExplorer();

private slots:

    void onGetListWallets(const wallets::WalletCurrency &type, const WalletsListCallback &callback);

    void onGetListWallets2(const wallets::WalletCurrency &type, const QString &expectedUsername, const WalletsListCallback &callback);

    void onGetWalletFolders(const GetWalletFoldersCallback &callback);

    void onBackupKeys(const QString &caption, const BackupKeysCallback &callback);

    void onRestoreKeys(const QString &caption, const RestoreKeysCallback &callback);

    void onOpenWalletPathInStandartExplorer();

///////////////
/// METHODS ///
///////////////

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

    std::vector<WalletInfo> readAllWallets(const WalletCurrency &type);

    int importKeysImpl(const QString &path, const std::function<bool(const QString &filePath)> &checkFileName, const std::function<void(const QString &path)> &processFile);

private:

    void findNonceAndProcessWithTxManager(const QString &address, const QString &nonce, const transactions::SendParameters &sendParams, const GettedNonceCallback &callback);

private:

    utils::Utils &utils;

    QString walletPath;

    QString userName;

    QString token;

    std::vector<FolderWalletInfo> folderWalletsInfos;

    QFileSystemWatcher fileSystemWatcher;

    EventWatcher eventWatcher;

    transactions::Transactions *txs = nullptr;

    std::map<WalletCurrency, std::map<QString, WalletInfo>> walletsList;

};

} // namespace wallets

#endif // WALLETS_WALLETS_H
