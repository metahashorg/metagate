#ifndef TRANSACTIONSMESSAGES_H
#define TRANSACTIONSMESSAGES_H

#include <QString>

#include <vector>

namespace transactions {

struct BalanceInfo;
struct Transaction;

QString makeGetBalanceRequest(const QString &address);

BalanceInfo parseBalanceResponse(const QString &response);

QString makeGetHistoryRequest(const QString &address, bool isCnt, uint64_t cnt);

QString makeGetTxRequest(const QString &hash);

std::vector<Transaction> parseHistoryResponse(const QString &address, const QString &currency, const QString &response);

QString makeSendTransactionRequest(QString to, QString value, size_t nonce, QString data, QString fee, QString pubkey, QString sign);

QString parseSendTransactionResponse(const QString &response);

Transaction parseGetTxResponse(const QString &response, const QString &address, const QString &currency);

}

#endif // TRANSACTIONSMESSAGES_H
