#ifndef TRANSACTIONSMESSAGES_H
#define TRANSACTIONSMESSAGES_H

#include <QString>

#include <vector>
#include <map>
#include <functional>

namespace transactions {

struct BalanceInfo;
struct Transaction;
struct BlockInfo;
struct SendParameters;

QByteArray makeGetBalanceRequest(const QString &address);

BalanceInfo parseBalanceResponse(const QByteArray &response);

QByteArray makeGetBalancesRequest(const std::vector<QString> &addresses);

void parseBalancesResponseWithHandler(const QByteArray &response, const std::function<void(const BalanceInfo &info)> &handler);

std::vector<BalanceInfo> parseBalancesResponse(const QByteArray &response);

std::map<QString, BalanceInfo> parseBalancesResponseToMap(const QByteArray &response);

QByteArray makeGetHistoryRequest(const QString &address, bool isCnt, qulonglong fromTx, qulonglong cnt);

QByteArray makeGetTxRequest(const QString &hash);

std::vector<Transaction> parseHistoryResponse(const QString &address, const QString &currency, const QByteArray &response);

QByteArray makeSendTransactionRequest(const QString &to, const QString &value, size_t nonce, const QString &data, const QString &fee, const QString &pubkey, const QString &sign);

QString parseSendTransactionResponse(const QByteArray &response);

Transaction parseGetTxResponse(const QByteArray &response, const QString &address, const QString &currency);

QByteArray makeGetBlockInfoRequest(int64_t blockNumber);

BlockInfo parseGetBlockInfoResponse(const QByteArray &response);

QByteArray makeGetCountBlocksRequest();

int64_t parseGetCountBlocksResponse(const QByteArray &response);

SendParameters parseSendParamsInternal(const QByteArray &paramsJson);

}

#endif // TRANSACTIONSMESSAGES_H
