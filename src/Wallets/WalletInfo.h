#ifndef WALLETS_WALLETINFO_H
#define WALLETS_WALLETINFO_H

#include <QString>

namespace wallets {

struct WalletInfo {
    enum class Type {
        Key,
        Watch
    };

    QString address;
    QString path;
    Type type;

    WalletInfo();

    WalletInfo(const QString &address, const QString &path, Type type);
};

} // namespace wallets

#endif // WALLETS_WALLETINFO_H
