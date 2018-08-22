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

struct BalanceInfo {
    QString address;
    QString received;
    QString spent;
    uint64_t countReceived = 0;
    uint64_t countSpent = 0;
    uint64_t currBlockNum = 0;
};

struct AddressInfo {
    QString currency;
    QString address;
    QString type;
    QString group;
    QString name;

    AddressInfo(const QString &currency, const QString &address, const QString &type, const QString &group, const QString &name)
        : currency(currency)
        , address(address)
        , type(type)
        , group(group)
        , name(name)
    {}

    AddressInfo() = default;
};

}

#endif // TRANSACTION_H
