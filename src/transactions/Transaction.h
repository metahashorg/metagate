#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QString>

namespace transactions {

struct Transaction {
    QString from;
    QString to;
    QString tx;
    uint64_t value;
    QString data;
    uint64_t timestamp;
    uint64_t fee;
    uint64_t nonce;
    bool isInput;
};

}

#endif // TRANSACTION_H
