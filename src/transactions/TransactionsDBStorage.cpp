#include "TransactionsDBStorage.h"

#include <QtSql>
#include <QDebug>

#include "TransactionsDBRes.h"
#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("TXS");

namespace transactions {

TransactionsDBStorage::TransactionsDBStorage(const QString &path)
    : DBStorage(path, databaseName)
{

}

int TransactionsDBStorage::currentVersion() const
{
    return databaseVersion;
}

void TransactionsDBStorage::addPayment(const QString &currency, const QString &txid, const QString &address, qint64 index,
                                       const QString &ufrom, const QString &uto, const QString &value,
                                       quint64 ts, const QString &data, const QString &fee, qint64 nonce,
                                       bool isSetDelegate, bool isDelegate, const QString &delegateValue, const QString &delegateHash,
                                       Transaction::Status status, Transaction::Type type, qint64 blockNumber, const QString &blockHash, int intStatus)
{
    QSqlQuery query(database());
    CHECK(query.prepare(insertPayment), query.lastError().text().toStdString());
    query.bindValue(":currency", currency);
    query.bindValue(":txid", txid);
    query.bindValue(":address", address);
    query.bindValue(":ind", index);
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
    query.bindValue(":delegateHash", delegateHash);
    query.bindValue(":status", status);
    query.bindValue(":type", type);
    query.bindValue(":blockNumber", blockNumber);
    query.bindValue(":blockHash", blockHash);
    query.bindValue(":intStatus", intStatus);
    CHECK(query.exec(), query.lastError().text().toStdString());

}

void TransactionsDBStorage::addPayment(const Transaction &trans)
{
    addPayment(trans.currency, trans.tx, trans.address, trans.blockIndex,
               trans.from, trans.to, trans.value,
               trans.timestamp, trans.data, trans.fee, trans.nonce,
               trans.isSetDelegate, trans.isDelegate, trans.delegateValue, trans.delegateHash,
               trans.status, trans.type, trans.blockNumber, trans.blockHash, trans.intStatus);
}

void TransactionsDBStorage::addPayments(const std::vector<Transaction> &transactions)
{
    auto transactionGuard = beginTransaction();
    for (const Transaction &transaction: transactions) {
        addPayment(transaction);
    }
    transactionGuard.commit();
}

std::vector<Transaction> TransactionsDBStorage::getPaymentsForAddress(const QString &address, const QString &currency,
                                                                      qint64 offset, qint64 count, bool asc)
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

std::vector<Transaction> TransactionsDBStorage::getPaymentsForAddressPending(const QString &address, const QString &currency, bool asc) const
{
    std::vector<Transaction> res;
    QSqlQuery query(database());
    CHECK(query.prepare(selectPaymentsForDestPending.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC"))),
          query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

std::vector<transactions::Transaction> transactions::TransactionsDBStorage::getForgingPaymentsForAddress(const QString &address, const QString &currency, qint64 offset, qint64 count, bool asc)
{
    std::vector<Transaction> res;
    QSqlQuery query(database());
    CHECK(query.prepare(selectForgingPaymentsForDest.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC")).arg(Transaction::FORGING)),
          query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":offset", offset);
    query.bindValue(":count", count);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

std::vector<Transaction> TransactionsDBStorage::getDelegatePaymentsForAddress(const QString &address, const QString &to, const QString &currency, qint64 offset, qint64 count, bool asc) {
    std::vector<Transaction> res;
    QSqlQuery query(database());
    CHECK(query.prepare(selectDelegatePaymentsForDest.arg(asc ? QStringLiteral("ASC") : QStringLiteral("DESC")).arg(Transaction::DELEGATE).arg(Transaction::Status::OK)),
          query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":to", to);
    query.bindValue(":currency", currency);
    query.bindValue(":offset", offset);
    query.bindValue(":count", count);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createPaymentsList(query, res);
    return res;
}

Transaction TransactionsDBStorage::getLastTransaction(const QString &address, const QString &currency) {
    Transaction trans;
    QSqlQuery query(database());
    CHECK(query.prepare(selectLastTransaction), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        setTransactionFromQuery(query, trans);
    }
    return trans;
}

Transaction TransactionsDBStorage::getLastForgingTransaction(const QString &address, const QString &currency)
{
    Transaction trans;
    QSqlQuery query(database());
    CHECK(query.prepare(selectLastForgingTransaction.arg(Transaction::FORGING)), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        setTransactionFromQuery(query, trans);
    }
    return trans;
}

void TransactionsDBStorage::updatePayment(const QString &address, const QString &currency, const QString &txid, qint64 blockNumber, qint64 index, const Transaction &trans)
{
    QSqlQuery query(database());
    CHECK(query.prepare(updatePaymentForAddress), query.lastError().text().toStdString())
            query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":txid", txid);
    query.bindValue(":blockNumber", blockNumber);
    query.bindValue(":ind", index);

    query.bindValue(":ufrom", trans.from);
    query.bindValue(":uto", trans.to);
    query.bindValue(":value", trans.value);
    query.bindValue(":ts", static_cast<qint64>(trans.timestamp));
    query.bindValue(":data", trans.data);
    query.bindValue(":fee", trans.fee);
    query.bindValue(":nonce", static_cast<qint64>(trans.nonce));
    query.bindValue(":isSetDelegate", trans.isSetDelegate);
    query.bindValue(":isDelegate", trans.isDelegate);
    query.bindValue(":delegateValue", trans.delegateValue);
    query.bindValue(":delegateHash", trans.delegateHash);
    query.bindValue(":status", trans.status);
    query.bindValue(":type", trans.type);
    query.bindValue(":blockHash", trans.blockHash);
    query.bindValue(":intStatus", trans.intStatus);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void TransactionsDBStorage::removePaymentsForDest(const QString &address, const QString &currency)
{
    QSqlQuery query(database());
    CHECK(query.prepare(deletePaymentsForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

qint64 TransactionsDBStorage::getPaymentsCountForAddress(const QString &address, const QString &currency) {
    QSqlQuery query(database());
    CHECK(query.prepare(selectPaymentsCountForAddress2), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("count").toLongLong();
    }
    return 0;
}

qint64 TransactionsDBStorage::getIsSetDelegatePaymentsCountForAddress(const QString &address, const QString &currency, Transaction::Status status)
{
    QSqlQuery query(database());
    CHECK(query.prepare(selectIsSetDelegatePaymentsCountForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":status", status);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("count").toLongLong();
    }
    return 0;
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

void TransactionsDBStorage::removePaymentsForCurrency(const QString &currency)
{
    auto transactionGuard = beginTransaction();
    QSqlQuery query(database());
    CHECK(query.prepare(removePaymentsForCurrencyQuery.arg(currency.isEmpty() ? QStringLiteral(""): removePaymentsCurrencyWhere)), query.lastError().text().toStdString());
    if (!currency.isEmpty())
        query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    CHECK(query.prepare(removeTrackedForCurrencyQuery.arg(currency.isEmpty() ? QStringLiteral(""): removePaymentsCurrencyWhere)), query.lastError().text().toStdString());
    if (!currency.isEmpty())
        query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    transactionGuard.commit();
}

void TransactionsDBStorage::setBalance(const QString &currency, const QString &address, const BalanceInfo &balance) {
    QSqlQuery queryDelete(database());
    CHECK(queryDelete.prepare(deleteBalance), queryDelete.lastError().text().toStdString());
    queryDelete.bindValue(":currency", currency);
    queryDelete.bindValue(":address", address);
    CHECK(queryDelete.exec(), queryDelete.lastError().text().toStdString());

    QSqlQuery query(database());
    CHECK(query.prepare(insertBalance), query.lastError().text().toStdString());
    query.bindValue(":currency", currency);
    query.bindValue(":address", address);
    query.bindValue(":received", balance.received.getDecimal());
    query.bindValue(":spent", balance.spent.getDecimal());
    query.bindValue(":countReceived", (qint64)balance.countReceived);
    query.bindValue(":countSpent", (qint64)balance.countSpent);
    query.bindValue(":countTxs", (qint64)balance.countTxs);
    query.bindValue(":currBlockNum", (qint64)balance.currBlockNum);
    query.bindValue(":countDelegated", (qint64)balance.countDelegated);
    query.bindValue(":delegate", balance.delegate.getDecimal());
    query.bindValue(":undelegate", balance.undelegate.getDecimal());
    query.bindValue(":delegated", balance.delegated.getDecimal());
    query.bindValue(":undelegated", balance.undelegated.getDecimal());
    query.bindValue(":reserved", balance.reserved.getDecimal());
    query.bindValue(":forged", balance.forged.getDecimal());
    CHECK(query.exec(), query.lastError().text().toStdString());
}

BalanceInfo TransactionsDBStorage::getBalance(const QString &currency, const QString &address) {
    QSqlQuery query(database());
    CHECK(query.prepare(selectBalance), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());

    BalanceInfo balance;
    balance.address = address;

    if (query.next()) {
        balance.received = query.value("received").toByteArray();
        balance.spent = query.value("spent").toByteArray();
        balance.countReceived = static_cast<quint64>(query.value("countReceived").toLongLong());
        balance.countSpent = static_cast<quint64>(query.value("countSpent").toLongLong());
        balance.countTxs = static_cast<quint64>(query.value("countTxs").toLongLong());
        balance.currBlockNum = static_cast<quint64>(query.value("currBlockNum").toLongLong());
        balance.countDelegated = static_cast<quint64>(query.value("countDelegated").toLongLong());
        balance.delegate = query.value("delegate").toByteArray();
        balance.undelegate = query.value("undelegate").toByteArray();
        balance.delegated = query.value("delegated").toByteArray();
        balance.undelegated = query.value("undelegated").toByteArray();
        balance.reserved = query.value("reserved").toByteArray();
        balance.forged = query.value("forged").toByteArray();
    }
    return balance;
}

void TransactionsDBStorage::createDatabase()
{
    createTable(QStringLiteral("payments"), createPaymentsTable);
    createTable(QStringLiteral("tracked"), createTrackedTable);
    createTable(QStringLiteral("balance"), createBalanceTable);
    createIndex(createPaymentsIndex1);
    createIndex(createPaymentsIndex2);
    createIndex(createPaymentsIndex3);
    createIndex(createPaymentsUniqueIndex);
    createIndex(createTrackedUniqueIndex);
}

void TransactionsDBStorage::setTransactionFromQuery(QSqlQuery &query, Transaction &trans) const
{
    trans.id = query.value("id").toLongLong();
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
    trans.isSetDelegate = query.value("isSetDelegate").toBool();
    trans.isDelegate = query.value("isDelegate").toBool();
    trans.delegateValue = query.value("delegateValue").toString();
    trans.delegateHash = query.value("delegateHash").toString();
    trans.status = static_cast<Transaction::Status>(query.value("status").toInt());
    trans.type = static_cast<Transaction::Type>(query.value("type").toInt());
    trans.blockNumber = query.value("blockNumber").toLongLong();
    trans.blockIndex = query.value("ind").toLongLong();
    trans.blockHash = query.value("blockHash").toString();
    trans.intStatus = query.value("intStatus").toInt();
}

void TransactionsDBStorage::createPaymentsList(QSqlQuery &query, std::vector<Transaction> &payments) const
{
    while (query.next()) {
        Transaction trans;
        setTransactionFromQuery(query, trans);
        payments.push_back(trans);
    }
}

}
