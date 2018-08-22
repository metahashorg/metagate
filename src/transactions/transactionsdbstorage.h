#ifndef TRANSACTIONSDBSTORAGE_H
#define TRANSACTIONSDBSTORAGE_H

#include "dbstorage.h"
#include "Transaction.h"
#include "BigNumber.h"
#include <list>

namespace transactions {

class TransactionsDBStorage : public DBStorage
{
public:
    TransactionsDBStorage();

    virtual void init() override;


    void addPayment(const QString &currency, const QString &txid, bool isInput,
                    const QString &ufrom, const QString &uto, const QString &value,
                    quint64 ts, const QString &data, qint64 fee, qint64 nonce);
    void addPayment(const Transaction &trans);

    std::list<Transaction> getPaymentsForDest(const QString &ufrom, const QString &uto, const QString &currency,
                                              qint64 offset, qint64 count, bool asc) const;
    std::list<Transaction> getPaymentsForCurrency(const QString &ufrom, const QString &currency,
                                                  qint64 offset, qint64 count, bool asc) const;

    void removePaymentsForDest(const QString &ufrom, const QString &uto, const QString &currency);

    qint64 getPaymentsCountForDest(const QString &ufrom, const QString &uto, const QString &currency, bool input);

    BigNumber calcInValueForDest(const QString &ufrom, const QString &uto, const QString &currency);
    BigNumber calcOutValueForDest(const QString &ufrom, const QString &uto, const QString &currency);

    void addTracked(const QString &currency, const QString &ufrom, const QString &uto, const QString &type, const QString &tgroup);

private:
    void createPaymentsList(QSqlQuery &query, std::list<Transaction> &payments) const;
};

}

#endif // TRANSACTIONSDBSTORAGE_H
