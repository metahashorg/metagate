#ifndef WALLETS_WALLETSJAVASCRIPT_H
#define WALLETS_WALLETSJAVASCRIPT_H

#include "qt_utilites/WrapperJavascript.h"

namespace wallets {

class Wallets;

class WalletsJavascript: public WrapperJavascript {
    Q_OBJECT
public:

    explicit WalletsJavascript(Wallets &wallets);

///////////
/// MHC ///
///////////

public:

    Q_INVOKABLE void createWallet(bool isMhc, const QString &password, const QString &callback);

    Q_INVOKABLE void createWalletWatch(bool isMhc, const QString &address, const QString &callback);

    Q_INVOKABLE void removeWalletWatch(bool isMhc, const QString &address, const QString &callback);

    Q_INVOKABLE void checkWalletExist(bool isMhc, const QString &address, const QString &callback);

    Q_INVOKABLE void checkWalletPassword(bool isMhc, const QString &address, const QString &password, const QString &callback);

    Q_INVOKABLE void checkWalletAddress(const QString &address, const QString &callback);

    Q_INVOKABLE void createContractAddress(const QString &address, int nonce, const QString &callback);

    Q_INVOKABLE void createTokenAddress(const QString &address, int nonce, const QString &callback);

    Q_INVOKABLE void signMessage(bool isMhc, const QString &address, const QString &text, const QString &password, const QString &callback);

    Q_INVOKABLE void signMessage2(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const QString &callback);

    Q_INVOKABLE void signAndSendMessage(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const QString &paramsJson, const QString &callback);

    Q_INVOKABLE void signAndSendMessageDelegate(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &valueDelegate, const QString &nonce, bool isDelegate, const QString &paramsJson, const QString &callback);

    Q_INVOKABLE void getOnePrivateKey(bool isMhc, const QString &address, bool isCompact, const QString &callback);

    Q_INVOKABLE void savePrivateKey(bool isMhc, const QString &privateKey, const QString &password, const QString &callback);

    Q_INVOKABLE void saveRawPrivateKey(bool isMhc, const QString &rawPrivateKey, const QString &password, const QString &callback);

    Q_INVOKABLE void getRawPrivateKey(bool isMhc, const QString &address, const QString &password, const QString &callback);

    Q_INVOKABLE void importKeys(bool isMhc, const QString &path, const QString &callback);

    Q_INVOKABLE void calkKeys(bool isMhc, const QString &path, const QString &callback);

private slots:

    void onWatchWalletsCreated(bool isMhc, const std::vector<std::pair<QString, QString>> &created);

///////////
/// RSA ///
///////////

public:

    Q_INVOKABLE void createRsaKey(bool isMhc, const QString &address, const QString &password, const QString &callback);

    Q_INVOKABLE void getRsaPublicKey(bool isMhc, const QString &address, const QString &callback);

    Q_INVOKABLE void copyRsaKey(bool isMhc, const QString &address, const QString &pathPub, const QString &pathPriv, const QString &callback);

    Q_INVOKABLE void copyRsaKeyToFolder(bool isMhc, const QString &address, const QString &path, const QString &callback);

////////////////
/// ETHEREUM ///
////////////////

public:

    Q_INVOKABLE void createEthKey(const QString &password, const QString &callback);

    Q_INVOKABLE void signMessageEth(const QString &address, const QString &password, const QString &nonce, const QString &gasPrice, const QString &gasLimit, const QString &to, const QString &value, const QString &data, const QString &callback);

    Q_INVOKABLE void checkAddressEth(const QString &address, const QString &callback);

    Q_INVOKABLE void savePrivateKeyEth(const QString &privateKey, const QString &password, const QString &callback);

    Q_INVOKABLE void getOnePrivateKeyEth(const QString &address, const QString &callback);

    Q_INVOKABLE void importKeysEth(const QString &path, const QString &callback);

    Q_INVOKABLE void calkKeysEth(const QString &path, const QString &callback);

///////////
/// BTC ///
///////////

public:

    Q_INVOKABLE void createBtcKey(const QString &password, const QString &callback);

    Q_INVOKABLE void checkAddressBtc(const QString &address, const QString &callback);

    Q_INVOKABLE void signMessageBtcUsedUtxos(const QString &address, const QString &password, const QString &jsonInputs, const QString &toAddress, const QString &value, const QString &estimateComissionInSatoshi, const QString &fees, const QString &jsonUsedUtxos, const QString &callback);

    Q_INVOKABLE void savePrivateKeyBtc(const QString &privateKey, const QString &password, const QString &callback);

    Q_INVOKABLE void getOnePrivateKeyBtc(const QString &address, const QString &callback);

    Q_INVOKABLE void importKeysBtc(const QString &path, const QString &callback);

    Q_INVOKABLE void calkKeysBtc(const QString &path, const QString &callback);

//////////////
/// COMMON ///
//////////////

public:

    Q_INVOKABLE void getWalletFolders(const QString &callback);

    Q_INVOKABLE void backupKeys(const QString &caption, const QString &callback);

    Q_INVOKABLE void restoreKeys(const QString &caption, const QString &callback);

    Q_INVOKABLE void openWalletPathInStandartExplorer();

private slots:

    void onDirChanged(const QString &absolutePath, const QString &nameCurrency);

private:

    Wallets &wallets;
};

} // namespace wallets

#endif // WALLETS_WALLETSJAVASCRIPT_H
