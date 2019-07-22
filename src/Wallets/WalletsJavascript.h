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

private:

    Wallets &wallets;
};

} // namespace wallets

#endif // WALLETS_WALLETSJAVASCRIPT_H
