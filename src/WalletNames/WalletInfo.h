#ifndef WALLETINFO_WALLET_NAMES_H
#define WALLETINFO_WALLET_NAMES_H

#include <QString>

#include <vector>

namespace wallet_names {

struct WalletInfo {
    struct Info {
        QString user = "";
        QString device = "";
        QString currency = "";

        Info(const QString &user, const QString &device, const QString &currency)
            : user(user)
            , device(device)
            , currency(currency)
        {}
    };

    QString address;
    QString name;

    std::vector<Info> infos;
};

} // namespace wallet_names

#endif // WALLETINFO_WALLET_NAMES_H
