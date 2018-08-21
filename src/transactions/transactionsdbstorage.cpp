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
}

void TransactionsDBStorage::addPayment(const QString &currency, const QString &txid, bool isInput,
                                       const QString &ufrom, const QString &uto, const QString &value,
                                       qint64 ts, const QString &data, qint64 fee, qint64 nonce)
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

std::list<Transaction> TransactionsDBStorage::getPaymentsForDest(const QString &ufrom, const QString &uto, const QString &txid, qint64 count, bool asc) const
{
    std::list<Transaction> res;
    QSqlQuery query(database());
    query.prepare(selectPaymentsForDest.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC")));
    query.bindValue(":ufrom", ufrom);
    query.bindValue(":uto", uto);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

std::list<Transaction> TransactionsDBStorage::getPaymentsForCurrency(const QString &ufrom, const QString &currency, const QString &txid, qint64 count, bool asc) const
{

}

void TransactionsDBStorage::createPaymentsList(QSqlQuery &query, std::list<Transaction> &payments) const
{
    while (query.next()) {
        Transaction trans;
        trans.from = query.value("ufrom").toString();
        trans.to = query.value("uto").toString();
        trans.tx = query.value("txid").toString();
        //trans.value = query.value("value").toString();
        trans.data = query.value("data").toString();
        trans.timestamp = static_cast<quint64>(query.value("ts").toLongLong());
        trans.fee = query.value("fee").toLongLong();
        trans.nonce = query.value("nonce").toLongLong();
        trans.isInput = query.value("isInput").toBool();
        payments.push_front(trans);
    }
}

}
