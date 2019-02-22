#include "TransactionsDBStorage.h"

#include <QtSql>
#include <QDebug>

#include "TransactionsDBRes.h"
#include "check.h"

namespace transactions {

TransactionsDBStorage::TransactionsDBStorage(const QString &path)
    : DBStorage(path, databaseName)
{

}

int TransactionsDBStorage::currentVersion() const
{
    return databaseVersion;
}

void TransactionsDBStorage::addPayment(const QString &currency, const QString &txid, const QString &address, bool isInput,
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
    addPayment(trans.currency, trans.tx, trans.address, trans.isInput,
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

Transaction TransactionsDBStorage::getLastPaymentIsSetDelegate(const QString &address, const QString &currency, const QString &from, const QString &to, bool isInput, bool isDelegate)
{
    Transaction trans;
    QSqlQuery query(database());
    CHECK(query.prepare(selectLastPaymentIsSetDelegate), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":ufrom", from);
    query.bindValue(":uto", to);
    query.bindValue(":isInput", isInput);
    query.bindValue(":isDelegate", isDelegate);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        setTransactionFromQuery(query, trans);
    }
    return trans;
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

void TransactionsDBStorage::updatePayment(const QString &address, const QString &currency, const QString &txid, bool isInput, const Transaction &trans)
{
    QSqlQuery query(database());
    CHECK(query.prepare(updatePaymentForAddress), query.lastError().text().toStdString())
            query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":txid", txid);
    query.bindValue(":isInput", isInput);

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
    query.bindValue(":blockNumber", static_cast<qint64>(trans.blockNumber));
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

BigNumber TransactionsDBStorage::calcIsSetDelegateValueForAddress(const QString &address, const QString &currency, bool isDelegate, bool isInput, Transaction::Status status)
{
    QSqlQuery query(database());
    CHECK(query.prepare(selectIsSetDelegatePaymentsValuesForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    query.bindValue(":isDelegate", isDelegate);
    query.bindValue(":isInput", isInput);
    query.bindValue(":status", status);
    CHECK(query.exec(), query.lastError().text().toStdString());
    BigNumber res;
    BigNumber r;
    while (query.next()) {
        r.setDecimal(query.value("delegateValue").toByteArray());
        res += r;
    }
    return res;
}

void TransactionsDBStorage::calcBalance(const QString &address, const QString &currency,
                                        BalanceInfo &balance)
{
    QSqlQuery query(database());
    CHECK(query.prepare(selectAllPaymentsValuesForAddress), query.lastError().text().toStdString());
    query.bindValue(":address", address);
    query.bindValue(":currency", currency);
    CHECK(query.exec(), query.lastError().text().toStdString());
    balance.received = BigNumber();
    balance.spent = BigNumber();
    balance.delegate = BigNumber();
    balance.undelegate = BigNumber();
    balance.delegated = BigNumber();
    balance.undelegated = BigNumber();
    balance.reserved = BigNumber();
    balance.forged = BigNumber();
    balance.countReceived = 0;
    balance.countSpent = 0;
    balance.countDelegated = 0;

    BigNumber value;
    BigNumber fee;
    BigNumber delegateValue;
    bool isSetDelegate;
    bool isDelegate;
    bool isInput;
    Transaction::Status status;
    Transaction::Type type;

    while (query.next()) {
        value.setDecimal(query.value("value").toByteArray());
        fee.setDecimal(query.value("fee").toByteArray());
        delegateValue.setDecimal(query.value("delegateValue").toByteArray());
        isSetDelegate = query.value("isSetDelegate").toBool();
        isDelegate = query.value("isDelegate").toBool();
        isInput = query.value("isInput").toBool();
        status = static_cast<Transaction::Status>(query.value("status").toInt());
        type = static_cast<Transaction::Type>(query.value("type").toInt());
        if (isInput) {
            balance.spent += value;
            balance.spent += fee;
            balance.countSpent++;
        } else {
            balance.received += value;
            balance.countReceived++;
        }
        if (isSetDelegate){
            balance.countDelegated++;
            if (status == Transaction::Status::OK) {
                balance.countDelegated++; // count transaction twice
                if (isInput && isDelegate) {
                    balance.delegate += delegateValue;
                } else if (!isInput && isDelegate) {
                    balance.delegated += delegateValue;
                } else if (isInput && !isDelegate) {
                    balance.undelegate += delegateValue;
                } else if (!isInput && !isDelegate) {
                    balance.undelegated += delegateValue;
                }
            }
            if (isInput && isDelegate  && status == Transaction::Status::PENDING) {
                balance.reserved += delegateValue;
            }
        }
        if (type == Transaction::Type::FORGING && !isInput) {
            balance.forged += value;
        }

    }
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

void TransactionsDBStorage::createDatabase()
{
    createTable(QStringLiteral("payments"), createPaymentsTable);
    createTable(QStringLiteral("tracked"), createTrackedTable);
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
    trans.isInput = query.value("isInput").toBool();
    trans.isSetDelegate = query.value("isSetDelegate").toBool();
    trans.isDelegate = query.value("isDelegate").toBool();
    trans.delegateValue = query.value("delegateValue").toString();
    trans.delegateHash = query.value("delegateHash").toString();
    trans.status = static_cast<Transaction::Status>(query.value("status").toInt());
    trans.type = static_cast<Transaction::Type>(query.value("type").toInt());
    trans.blockNumber = query.value("blockNumber").toLongLong();
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
