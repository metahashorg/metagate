#ifndef TRANSACTIONSMESSAGES_H
#define TRANSACTIONSMESSAGES_H

#include <QString>

#include <vector>

namespace transactions {

struct BalanceResponse {
    QString address;
    uint64_t received = 0;
    uint64_t spent = 0;
    uint64_t countReceived = 0;
    uint64_t countSpent = 0;
    uint64_t currBlockNum = 0;
};

struct TxResponse {
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

QString makeGetBalanceRequest(const QString &address);

BalanceResponse parseBalanceResponse(const QString &response);

QString makeGetHistoryRequest(const QString &address, bool isCnt, uint64_t cnt);

std::vector<TxResponse> parseHistoryResponse(const QString &address, const QString &response);

}

#endif // TRANSACTIONSMESSAGES_H
