#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QString>

#include "BigNumber.h"
#include "dbstorage.h"

namespace transactions {

enum class DelegateStatus {
    NOT_FOUND, PENDING, ERROR, DELEGATE, UNDELEGATE
};

struct Transaction {
    enum Status {
        OK = 0, PENDING = 1, ERROR = 2
    };

    enum Type {
        SIMPLE = 0, FORGING = 1, DELEGATE = 2
    };

    DBStorage::DbId id = -1;
    QString currency;
    QString tx;
    QString address;
    QString from;
    QString to;
    QString value;
    QString data;
    uint64_t timestamp;
    QString fee;
    int64_t nonce = 0;
    bool isInput;
    int64_t blockNumber = 0;
    QString blockHash = "";

    bool isSetDelegate = false; // TODO после введения type стало избыточным полем. Удалить
    bool isDelegate;
    QString delegateValue;
    QString delegateHash;

    Type type = Type::SIMPLE;

    Status status = Status::OK;
};

struct BalanceInfo {
    QString address;
    BigNumber received;
    BigNumber spent;
    uint64_t countReceived = 0;
    uint64_t countSpent = 0;
    uint64_t currBlockNum = 0;

    uint64_t countDelegated;
    BigNumber delegate;
    BigNumber undelegate;
    BigNumber delegated;
    BigNumber undelegated;
    BigNumber reserved = QString("0");
    BigNumber forged = QString("0");

    BigNumber calcBalance() const {
        return received - spent - reserved;
    }
};

struct AddressInfo {
    QString currency;
    QString address;
    QString type;
    QString group;
    QString name;

    BalanceInfo balance;

    AddressInfo(const QString &currency, const QString &address, const QString &type, const QString &group, const QString &name)
        : currency(currency)
        , address(address)
        , type(type)
        , group(group)
        , name(name)
    {}

    AddressInfo() = default;
};

struct BlockInfo {
    QString hash;
    int64_t number;
};

}

#endif // TRANSACTION_H
