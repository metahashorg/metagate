#include "WalletNamesDbStorage.h"

#include <QtSql>

#include "Log.h"
#include "check.h"

#include "WalletNamesDbRes.h"

SET_LOG_NAMESPACE("WNS");

namespace wallet_names {

WalletNamesDbStorage::WalletNamesDbStorage(const QString &path)
    : DBStorage(path, databaseName)
{

}

int WalletNamesDbStorage::currentVersion() const {
    return databaseVersion;
}

void WalletNamesDbStorage::createDatabase() {
    createTable(QStringLiteral("wallets"), createWalletsTable);
    createTable(QStringLiteral("info"), createInfoTable);
    createIndex(createWalletsUniqueIndex);
    createIndex(createInfoUniqueIndex);
}

bool WalletNamesDbStorage::giveNameWallet(const QString &address, const QString &name) {
    QSqlQuery query(database());
    CHECK(query.prepare(selectName), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    CHECK(query.exec(), query.lastError().text().toStdString());
    const bool ifExist = query.next();
    if (!ifExist) {
        CHECK(query.prepare(giveNameWalletAdd), query.lastError().text().toStdString());
        query.bindValue(":address", address);
        query.bindValue(":name", name);
        CHECK(query.exec(), query.lastError().text().toStdString());
        return false;
    } else {
        const QString oldValue = query.value("name").toString();
        if (oldValue != name) {
            CHECK(query.prepare(giveNameWalletRename), query.lastError().text().toStdString());
            query.bindValue(":address", address);
            query.bindValue(":name", name);
            CHECK(query.exec(), query.lastError().text().toStdString());
            return true;
        } else {
            return false;
        }
    }
}

std::vector<WalletInfo> WalletNamesDbStorage::getAllWallets() {
    QSqlQuery query(database());
    CHECK(query.prepare(selectAll), query.lastError().text().toStdString());
    CHECK(query.exec(), query.lastError().text().toStdString());

    return createWalletsList(query);
}

void WalletNamesDbStorage::updateWalletInfo(const QString &address, const std::vector<WalletInfo::Info> &infos) {
    QSqlQuery query(database());
    CHECK(query.prepare(insertWalletInfo), query.lastError().text().toStdString());
    for (const WalletInfo::Info &i: infos) {
        query.bindValue(":address", address);
        query.bindValue(":user", i.user);
        query.bindValue(":device", i.device);
        query.bindValue(":currency", i.currency);
        CHECK(query.exec(), query.lastError().text().toStdString());
    }
}

std::vector<WalletInfo> WalletNamesDbStorage::createWalletsList(QSqlQuery &query) {
    std::map<QString, WalletInfo> result;
    while (query.next()) {
        const QString address = query.value("address").toString();
        WalletInfo &info = result[address];

        info.address = address;
        info.name = query.value("name").toString();

        const QString user = query.value("user").toString();
        const QString device = query.value("device").toString();
        const QString currency = query.value("currency").toString();
        if (!user.isEmpty() || !device.isEmpty() || !currency.isEmpty()) {
            info.infos.emplace_back(user, device, currency);
        }
    }
    std::vector<WalletInfo> res;
    res.reserve(result.size());
    using MapIter = std::decay_t<decltype(*result.begin())>;
    std::transform(result.begin(), result.end(), std::back_inserter(res), std::mem_fn(&MapIter::second));
    return res;
}

std::vector<WalletInfo> WalletNamesDbStorage::getWalletsCurrency(const QString &currency, const QString &user) {
    QSqlQuery query(database());
    CHECK(query.prepare(selectForCurrencyAndUser), query.lastError().text().toStdString());
    query.bindValue(":currency", currency);
    query.bindValue(":user", user);
    CHECK(query.exec(), query.lastError().text().toStdString());

    return createWalletsList(query);
}

bool WalletNamesDbStorage::addOrUpdateWallet(const WalletInfo &info) {
    const bool isUpdated = giveNameWallet(info.address, info.name);
    updateWalletInfo(info.address, info.infos);
    return isUpdated;
}

QString WalletNamesDbStorage::getNameWallet(const QString &address) {
    QSqlQuery query(database());
    CHECK(query.prepare(selectName), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    CHECK(query.exec(), query.lastError().text().toStdString());

    if (query.next()) {
        return query.value("name").toString();
    } else {
        return "";
    }
}

WalletInfo WalletNamesDbStorage::getWalletInfo(const QString &address) {
    QSqlQuery query(database());
    CHECK(query.prepare(selectInfo), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    CHECK(query.exec(), query.lastError().text().toStdString());

    const std::vector<WalletInfo> res = createWalletsList(query);
    if (res.empty()) {
        return WalletInfo();
    } else {
        return res[0];
    }
}

} // namespace wallet_names
