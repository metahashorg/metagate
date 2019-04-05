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

void WalletNamesDbStorage::giveNameWallet(const QString &address, const QString &name) {
    QSqlQuery query(database());
    CHECK(query.prepare(giveNameWalletPart1), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":name", name);
    CHECK(query.exec(), query.lastError().text().toStdString());

    CHECK(query.prepare(giveNameWalletPart2), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":name", name);
    CHECK(query.exec(), query.lastError().text().toStdString());
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
        info.infos.emplace_back(query.value("user").toString(), query.value("device").toString(), query.value("currency").toString());
    }
    std::vector<WalletInfo> res;
    res.reserve(result.size());
    using MapIter = std::decay_t<decltype(*result.begin())>;
    std::transform(result.begin(), result.end(), std::back_inserter(res), std::mem_fn(&MapIter::second));
    return res;
}

void WalletNamesDbStorage::addOrUpdateWallet(const WalletInfo &info) {
    giveNameWallet(info.address, info.name);
    updateWalletInfo(info.address, info.infos);
}

} // namespace wallet_names
