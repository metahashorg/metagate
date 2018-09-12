#include "transactionsdbstorage.h"

#include <QtSql>
#include <QDebug>

#include "transactionsdbres.h"
#include "check.h"

namespace transactions {

TransactionsDBStorage::TransactionsDBStorage(const QString &path)
    : DBStorage(path, databaseFileName)
{

}

void transactions::TransactionsDBStorage::init(bool force)
{
    if (dbExist() && !force)
        return;
    DBStorage::init(force);
    createTable(QStringLiteral("payments"), createPaymentsTable);
    createTable(QStringLiteral("tracked"), createTrackedTable);
    createIndex(createPaymentsSortingIndex);
    createIndex(createPaymentsUniqueIndex);
    createIndex(createTrackedUniqueIndex);
}

void TransactionsDBStorage::addPayment(const QString &currency, const QString &txid, const QString &address, bool isInput,
                                       const QString &ufrom, const QString &uto, const QString &value,
                                       quint64 ts, const QString &data, const QString &fee, qint64 nonce,
                                       bool isSetDelegate, bool isDelegate, QString delegateValue)
{
    QSqlQuery query(database());
    CHECK(query.prepare(insertPayment), query.lastError().text().toStdString());
    query.bindValue(":currency", currency);
    query.bindValue(":txid", txid);
    query.bindValue(":address", address);
    query.bindValue(":isInput", isInput);
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":uto", uto);
    query.bindValue(":value", value);
    query.bindValue(":ts", ts);
    query.bindValue(":data", data);
    query.bindValue(":fee", fee);
    query.bindValue(":nonce", nonce);
    query.bindValue(":isSetDelegate", isSetDelegate);
    query.bindValue(":isDelegate", isDelegate);
    query.bindValue(":delegateValue", delegateValue);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void TransactionsDBStorage::addPayment(const Transaction &trans)
{
    addPayment(trans.currency, trans.tx, trans.address, trans.isInput,
               trans.from, trans.to, trans.value,
               trans.timestamp, trans.data, trans.fee, trans.nonce,
               trans.isSetDelegate, trans.isDelegate, trans.delegateValue);
}

std::vector<Transaction> TransactionsDBStorage::getPaymentsForAddress(const QString &address, const QString &currency,
                                                                 qint64 offset, qint64 count, bool asc) const
{
    std::vector<Transaction> res;
    QSqlQuery query(database());
    CHECK(query.prepare(selectPaymentsForDest.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC"))),
          query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":offset", offset);
    query.bindValue(":count", count);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

std::vector<Transaction> TransactionsDBStorage::getPaymentsForCurrency(const QString &currency,
                                                                     qint64 offset, qint64 count, bool asc) const
{
    std::vector<Transaction> res;
    QSqlQuery query(database());
    CHECK(query.prepare(selectPaymentsForCurrency.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC"))),
          query.lastError().text().toStdString());
    query.bindValue(":currency", currency);
    query.bindValue(":offset", offset);
    query.bindValue(":count", count);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

void TransactionsDBStorage::removePaymentsForDest(const QString &address, const QString &currency)
{
    QSqlQuery query(database());
    CHECK(query.prepare(deletePaymentsForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

qint64 TransactionsDBStorage::getPaymentsCountForAddress(const QString &address, const QString &currency, bool input)
{
    QSqlQuery query(database());
    CHECK(query.prepare(selectPaymentsCountForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":input", input);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("count").toLongLong();
    }
    return 0;
}

BigNumber TransactionsDBStorage::calcInValueForAddress(const QString &address, const QString &currency)
{
    QSqlQuery query(database());
    CHECK(query.prepare(selectInPaymentsValuesForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    BigNumber res;
    BigNumber r;
    while (query.next()) {
        r.setDecimal(query.value("value").toByteArray());
        res += r;
        r.setDecimal(query.value("fee").toByteArray());
        res += r;
    }
    return res;
}

BigNumber TransactionsDBStorage::calcOutValueForAddress(const QString &address, const QString &currency)
{
    QSqlQuery query(database());
    CHECK(query.prepare(selectOutPaymentsValuesForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    BigNumber res;
    BigNumber r;
    while (query.next()) {
        r.setDecimal(query.value("value").toByteArray());
        res += r;
    }
    return res;
}

qint64 TransactionsDBStorage::getIsSetDelegatePaymentsCountForAddress(const QString &address, const QString &currency)
{
    QSqlQuery query(database());
    CHECK(query.prepare(selectIsSetDelegatePaymentsCountForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("count").toLongLong();
    }
    return 0;
}

BigNumber TransactionsDBStorage::calcIsSetDelegateValueForAddress(const QString &address, const QString &currency, bool isDelegate, bool isInput)
{
    QSqlQuery query(database());
    CHECK(query.prepare(selectIsSetDelegatePaymentsValuesForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":isDelegate", isDelegate);
    query.bindValue(":isInput", isInput);
    CHECK(query.exec(), query.lastError().text().toStdString());
    BigNumber res;
    BigNumber r;
    while (query.next()) {
        r.setDecimal(query.value("value").toByteArray());
        res += r;
    }
    return res;
}

void TransactionsDBStorage::addTracked(const QString &currency, const QString &address, const QString &name, const QString &type, const QString &tgroup)
{
    QSqlQuery query(database());
    CHECK(query.prepare(insertTracked), query.lastError().text().toStdString());
    query.bindValue(":currency", currency);
    query.bindValue(":address", address);
    query.bindValue(":name", name);
    query.bindValue(":type", type);
    query.bindValue(":tgroup", tgroup);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void TransactionsDBStorage::addTracked(const AddressInfo &info)
{
    addTracked(info.currency, info.address, info.name, info.type, info.group);
}

std::vector<AddressInfo> TransactionsDBStorage::getTrackedForGroup(const QString &tgroup)
{
    std::vector<AddressInfo> res;
    QSqlQuery query(database());
    CHECK(query.prepare(selectTrackedForGroup), query.lastError().text().toStdString());
    query.bindValue(":tgroup", tgroup);
    CHECK(query.exec(), query.lastError().text().toStdString());
    while (query.next()) {
        AddressInfo info(query.value("currency").toString(),
                         query.value("address").toString(),
                         query.value("type").toString(),
                         tgroup,
                         query.value("name").toString()
                         );
        res.push_back(info);
    }
    return res;
}

void TransactionsDBStorage::createPaymentsList(QSqlQuery &query, std::vector<Transaction> &payments) const
{
    while (query.next()) {
        Transaction trans;
        trans.currency = query.value("currency").toString();
        trans.address = query.value("address").toString();
        trans.tx = query.value("txid").toString();
        trans.from = query.value("ufrom").toString();
        trans.to = query.value("uto").toString();
        trans.value = query.value("value").toString();
        trans.data = query.value("data").toString();
        trans.timestamp = static_cast<quint64>(query.value("ts").toLongLong());
        trans.fee = query.value("fee").toString();
        trans.nonce = query.value("nonce").toLongLong();
        trans.isInput = query.value("isInput").toBool();
        payments.push_back(trans);
    }
}

}
