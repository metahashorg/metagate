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

    Q_INVOKABLE void createWallet(bool isMhc, QString password);

private:

    Wallets &wallets;
};

} // namespace wallets

#endif // WALLETS_WALLETSJAVASCRIPT_H
