#ifndef WALLETINFO_WALLET_NAMES_H
#define WALLETINFO_WALLET_NAMES_H

#include <QString>

#include <vector>

namespace wallet_names {

struct WalletInfo {
    struct Info {
        enum class Type {
            Watch, Key
        };

        QString user = "";
        QString device = "";
        QString currency = "";
        Type type = Type::Key;

        Info() = default;

        Info(const QString &user, const QString &device, const QString &currency, Type type)
            : user(user)
            , device(device)
            , currency(currency)
            , type(type)
        {}
    };

    QString address;
    QString name;

    std::vector<Info> infos;

    Info currentInfo;
};

} // namespace wallet_names

#endif // WALLETINFO_WALLET_NAMES_H
