#ifndef WALLETS_WALLETSJAVASCRIPT_H
#define WALLETS_WALLETSJAVASCRIPT_H

#include "qt_utilites/WrapperJavascript.h"

namespace wallets {

class Wallets;

class WalletsJavascript: public WrapperJavascript {
    Q_OBJECT
public:

    explicit WalletsJavascript(Wallets &wallets, QObject *parent = nullptr);

private slots:

    void onWatchWalletsCreated(bool isMhc, const std::vector<std::pair<QString, QString>> &created);

public:

    Q_INVOKABLE void createWallet(bool isMhc, const QString &password);

    Q_INVOKABLE void createWalletWatch(bool isMhc, const QString &address);

    Q_INVOKABLE void removeWalletWatch(bool isMhc, const QString &address);

    Q_INVOKABLE void checkWalletExist(bool isMhc, const QString &address);

    Q_INVOKABLE void checkWalletPassword(bool isMhc, const QString &address, const QString &password);

    Q_INVOKABLE void checkWalletAddress(const QString &address);

    Q_INVOKABLE void createContractAddress(const QString &address, int nonce);

    Q_INVOKABLE void signMessage(bool isMhc, const QString &address, const QString &text, const QString &password);

    Q_INVOKABLE void signMessage2(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex);

    Q_INVOKABLE void signAndSendMessage(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const QString &paramsJson);

    Q_INVOKABLE void signAndSendMessageDelegate(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &valueDelegate, const QString &nonce, bool isDelegate, const QString &paramsJson);

    Q_INVOKABLE void getOnePrivateKey(bool isMhc, const QString &address, bool isCompact);

    Q_INVOKABLE void savePrivateKey(bool isMhc, const QString &privateKey, const QString &password);

    Q_INVOKABLE void saveRawPrivateKey(bool isMhc, const QString &rawPrivateKey, const QString &password);

    Q_INVOKABLE void getRawPrivateKey(bool isMhc, const QString &address, const QString &password);

private:

    Wallets &wallets;
};

} // namespace wallets

#endif // WALLETS_WALLETSJAVASCRIPT_H
