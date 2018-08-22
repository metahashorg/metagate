#include "transactionsdbstorage.h"

#include <QtSql>
#include <QDebug>

#include "transactionsdbres.h"
#include "check.h"

namespace transactions {

TransactionsDBStorage::TransactionsDBStorage()
    : DBStorage(databaseFileName)
{

}

void transactions::TransactionsDBStorage::init()
{
    if (dbExist())
        return;
    DBStorage::init();
    createTable(QStringLiteral("payments"), createPaymentsTable);
    createTable(QStringLiteral("tracked"), createTrackedTable);
    createIndex(createPaymentsSortingIndex);
}

void TransactionsDBStorage::addPayment(const QString &currency, const QString &txid, const QString &address, bool isInput,
                                       const QString &ufrom, const QString &uto, const QString &value,
                                       quint64 ts, const QString &data, qint64 fee, qint64 nonce)
{
    QSqlQuery query(database());
    query.prepare(insertPayment);
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
    if (!query.exec()) {
        qDebug() << "ERROR" << query.lastError().type();
    }
}

void TransactionsDBStorage::addPayment(const Transaction &trans)
{
    addPayment(trans.currency, trans.tx, trans.address, trans.isInput,
               trans.from, trans.to, trans.value,
               trans.timestamp, trans.data, trans.fee, trans.nonce);
}

std::list<Transaction> TransactionsDBStorage::getPaymentsForAddress(const QString &address, const QString &currency,
                                                                 qint64 offset, qint64 count, bool asc) const
{
    std::list<Transaction> res;
    QSqlQuery query(database());
    query.prepare(selectPaymentsForDest.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC")));
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":offset", offset);
    query.bindValue(":count", count);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

std::list<Transaction> TransactionsDBStorage::getPaymentsForCurrency(const QString &currency,
                                                                     qint64 offset, qint64 count, bool asc) const
{
    std::list<Transaction> res;
    QSqlQuery query(database());
    query.prepare(selectPaymentsForCurrency.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC")));
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
    query.prepare(deletePaymentsForAddress);
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

qint64 TransactionsDBStorage::getPaymentsCountForAddress(const QString &address, const QString &currency, bool input)
{
    QSqlQuery query(database());
    query.prepare(selectPaymentsCountForAddress);
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
    query.prepare(selectInPaymentsValuesForAddress);
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

BigNumber TransactionsDBStorage::calcOutValueForAddress(const QString &address, const QString &currency)
{
    QSqlQuery query(database());
    query.prepare(selectOutPaymentsValuesForAddress);
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

void TransactionsDBStorage::addTracked(const QString &currency, const QString &address,
                                       const QString &type, const QString &tgroup)
{
    QSqlQuery query(database());
    query.prepare(insertTracked);
    query.bindValue(":currency", currency);
    query.bindValue(":address", address);
    query.bindValue(":type", type);
    query.bindValue(":tgroup", tgroup);
    if (!query.exec()) {
        qDebug() << "ERROR" << query.lastError().type();
    }
}

std::list<AddressInfo> TransactionsDBStorage::getTrackedForGroup(const QString &tgroup)
{
    std::list<AddressInfo> res;
    QSqlQuery query(database());
    query.prepare(selectTrackedForGroup);
    query.bindValue(":tgroup", tgroup);
    CHECK(query.exec(), query.lastError().text().toStdString());
    while (query.next()) {
        AddressInfo info(query.value("currency").toString(),
                         query.value("address").toString(),
                         query.value("type").toString(),
                         tgroup,
                         query.value("name").toString()
                         );
        res.push_front(info);
    }
    return res;
}

void TransactionsDBStorage::createPaymentsList(QSqlQuery &query, std::list<Transaction> &payments) const
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
        trans.fee = query.value("fee").toLongLong();
        trans.nonce = query.value("nonce").toLongLong();
        trans.isInput = query.value("isInput").toBool();
        payments.push_front(trans);
    }
}

}
