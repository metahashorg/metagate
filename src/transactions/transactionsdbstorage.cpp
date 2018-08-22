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
    //createTable(QStringLiteral("payments"), createPaymentsTable);
    createTable(QStringLiteral("tracked"), createTrackedTable);
    createIndex(createPaymentsSortingIndex);
}

void TransactionsDBStorage::addPayment(const QString &currency, const QString &txid, bool isInput,
                                       const QString &ufrom, const QString &uto, const QString &value,
                                       quint64 ts, const QString &data, qint64 fee, qint64 nonce)
{
    QSqlQuery query(database());
    query.prepare(insertPayment);
    query.bindValue(":currency", currency);
    query.bindValue(":txid", txid);
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
    addPayment(trans.currency, trans.tx, trans.isInput,
               trans.from, trans.to, trans.value,
               trans.timestamp, trans.data, trans.fee, trans.nonce);
}

std::list<Transaction> TransactionsDBStorage::getPaymentsForDest(const QString &ufrom, const QString &uto, const QString &currency,
                                                                 qint64 offset, qint64 count, bool asc) const
{
    std::list<Transaction> res;
    QSqlQuery query(database());
    query.prepare(selectPaymentsForDest.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC")));
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":uto", uto);
    query.bindValue(":currency", currency);
    query.bindValue(":offset", offset);
    query.bindValue(":count", count);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

std::list<Transaction> TransactionsDBStorage::getPaymentsForCurrency(const QString &ufrom, const QString &currency,
                                                                     qint64 offset, qint64 count, bool asc) const
{
    std::list<Transaction> res;
    QSqlQuery query(database());
    query.prepare(selectPaymentsForCurrency.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC")));
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":currency", currency);
    query.bindValue(":offset", offset);
    query.bindValue(":count", count);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

void TransactionsDBStorage::removePaymentsForDest(const QString &ufrom, const QString &uto, const QString &currency)
{
    QSqlQuery query(database());
    query.prepare(deletePaymentsForDest);
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":uto", uto);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

qint64 TransactionsDBStorage::getPaymentsCountForDest(const QString &ufrom, const QString &uto, const QString &currency, bool input)
{
    QSqlQuery query(database());
    query.prepare(selectPaymentsCountForDest);
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":uto", uto);
    query.bindValue(":currency", currency);
    query.bindValue(":input", input);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("count").toLongLong();
    }
    return 0;
}

BigNumber TransactionsDBStorage::calcInValueForDest(const QString &ufrom, const QString &uto, const QString &currency)
{
    QSqlQuery query(database());
    query.prepare(selectInPaymentsValuesForDest);
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":uto", uto);
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

BigNumber TransactionsDBStorage::calcOutValueForDest(const QString &ufrom, const QString &uto, const QString &currency)
{
    QSqlQuery query(database());
    query.prepare(selectOutPaymentsValuesForDest);
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":uto", uto);
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

void TransactionsDBStorage::addTracked(const QString &currency, const QString &ufrom, const QString &uto,
                                       const QString &type, const QString &tgroup)
{
    QSqlQuery query(database());
    query.prepare(insertTracked);
    query.bindValue(":currency", currency);
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":uto", uto);
    query.bindValue(":type", type);
    query.bindValue(":tgroup", tgroup);
    if (!query.exec()) {
        qDebug() << "ERROR" << query.lastError().type();
    }
}

void TransactionsDBStorage::createPaymentsList(QSqlQuery &query, std::list<Transaction> &payments) const
{
    while (query.next()) {
        Transaction trans;
        trans.from = query.value("ufrom").toString();
        trans.to = query.value("uto").toString();
        trans.tx = query.value("txid").toString();
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
