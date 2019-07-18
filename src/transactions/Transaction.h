#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <QString>

#include "utilites/BigNumber.h"
#include "dbstorage.h"
#include "duration.h"

namespace transactions {

const quint32 BNModule = 6;
const QString TorrentTypeMainNet = QStringLiteral("torrent_main");
const QString TorrentTypeDevNet = QStringLiteral("torrent");



enum class DelegateStatus {
    NOT_FOUND, PENDING, ERROR, DELEGATE, UNDELEGATE
};

struct Transaction {
    enum Status {
        OK = 0, PENDING = 1, ERROR = 2, MODULE_NOT_SET = 3
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
    int64_t blockNumber = 0;
    int64_t blockIndex = 0;
    QString blockHash = "";
    int intStatus = 0;

    bool isSetDelegate = false; // TODO после введения type стало избыточным полем. Удалить
    bool isDelegate;
    QString delegateValue;
    QString delegateHash;

    Type type = Type::SIMPLE;

    Status status = Status::OK;
};

struct BalanceInfo {
    QString address;
    BigNumber received = QString("0");
    BigNumber spent = QString("0");
    uint64_t countReceived = 0;
    uint64_t countSpent = 0;
    uint64_t countTxs = 0;
    uint64_t currBlockNum = 0;
    uint64_t savedTxs = 0;

    uint64_t countDelegated = 0;
    BigNumber delegate = QString("0");
    BigNumber undelegate = QString("0");
    BigNumber delegated = QString("0");
    BigNumber undelegated = QString("0");
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
    QString name = "";

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

struct SendParameters {
    size_t countServersSend;
    size_t countServersGet;
    QString typeSend;
    QString typeGet;
    seconds timeout;
};

}

#endif // TRANSACTION_H
