#ifndef TRANSACTIONSMESSAGES_H
#define TRANSACTIONSMESSAGES_H

#include <QString>

#include <vector>

namespace transactions {

struct BalanceInfo;
struct Transaction;
struct BlockInfo;
struct SendParameters;

QString makeGetBalanceRequest(const QString &address);

BalanceInfo parseBalanceResponse(const QString &response);

QString makeGetBalancesRequest(const std::vector<QString> &addresses);

std::vector<BalanceInfo> parseBalancesResponse(const QString &response);

QString makeGetHistoryRequest(const QString &address, bool isCnt, uint64_t cnt);

QString makeGetTxRequest(const QString &hash);

std::vector<Transaction> parseHistoryResponse(const QString &address, const QString &currency, const QString &response);

QString makeSendTransactionRequest(const QString &to, const QString &value, size_t nonce, const QString &data, const QString &fee, const QString &pubkey, const QString &sign);

QString parseSendTransactionResponse(const QString &response);

Transaction parseGetTxResponse(const QString &response, const QString &address, const QString &currency);

QString makeGetBlockInfoRequest(int64_t blockNumber);

BlockInfo parseGetBlockInfoResponse(const QString &response);

QString makeGetCountBlocksRequest();

int64_t parseGetCountBlocksResponse(const QString &response);

SendParameters parseSendParamsInternal(const QString &paramsJson);

}

#endif // TRANSACTIONSMESSAGES_H
