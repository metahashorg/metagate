#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QString>

namespace transactions {

struct Transaction {
    QString currency;
    QString from;
    QString to;
    QString tx;
    QString value;
    QString data;
    uint64_t timestamp;
    int64_t fee;
    int64_t nonce;
    bool isInput;
};

struct BalanceResponse {
    QString address;
    QString received;
    QString spent;
    uint64_t countReceived = 0;
    uint64_t countSpent = 0;
    uint64_t currBlockNum = 0;
};

}

#endif // TRANSACTION_H
