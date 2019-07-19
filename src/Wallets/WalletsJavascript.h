#ifndef WALLETS_WALLETSJAVASCRIPT_H
#define WALLETS_WALLETSJAVASCRIPT_H

#include "qt_utilites/WrapperJavascript.h"

namespace wallets {

class Wallets;

class WalletsJavascript: public WrapperJavascript {
    Q_OBJECT
public:
    explicit WalletsJavascript(Wallets &wallets, QObject *parent = nullptr);

private:

    Wallets &wallets;
};

} // namespace wallets

#endif // WALLETS_WALLETSJAVASCRIPT_H
