#ifndef WALLETNAMESDBSTORAGE_H
#define WALLETNAMESDBSTORAGE_H

#include "dbstorage.h"

#include "WalletInfo.h"

namespace wallet_names {

class WalletNamesDbStorage: public DBStorage {
public:

    WalletNamesDbStorage(const QString &path = QString());

    int currentVersion() const final override;

public:

    void giveNameWallet(const QString &address, const QString &name);

    void updateWalletInfo(const QString &address, const std::vector<WalletInfo::Info> &infos);

    void addOrUpdateWallet(const WalletInfo &info);

    QString getNameWallet(const QString &address);

    WalletInfo getWalletInfo(const QString &address);

    std::vector<WalletInfo> getWalletsCurrency(const QString &currency, const QString &user);

    std::vector<WalletInfo> getAllWallets();

protected:

    void createDatabase() final override;

private:

    std::vector<WalletInfo> createWalletsList(QSqlQuery &query);

};

} // namespace wallet_names

#endif // WALLETNAMESDBSTORAGE_H
