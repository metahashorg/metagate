#ifndef TRANSACTIONSDBSTORAGE_H
#define TRANSACTIONSDBSTORAGE_H

#include "dbstorage.h"
#include "Transaction.h"
#include <list>

namespace transactions {

class TransactionsDBStorage : public DBStorage
{
public:
    TransactionsDBStorage();

    virtual void init() override;


    void addPayment(const QString &currency, const QString &txid, bool isInput,
                    const QString &ufrom, const QString &uto, const QString &value,
                    qint64 ts, const QString &data, qint64 fee, qint64 nonce);

    std::list<Transaction> getPaymentsForDest(const QString &ufrom, const QString &uto, const QString &txid, qint64 count, bool asc) const;
    std::list<Transaction> getPaymentsForCurrency(const QString &ufrom, const QString &currency, const QString &txid, qint64 count, bool asc) const;

private:
    void createPaymentsList(QSqlQuery &query, std::list<Transaction> &payments) const;
};

}

#endif // TRANSACTIONSDBSTORAGE_H
