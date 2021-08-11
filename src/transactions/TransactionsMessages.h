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
struct TokenBalance;
struct Token;

QString makeGetBalanceRequest(const QString &address);

BalanceInfo parseBalanceResponse(const QString &response);

QString makeGetBalancesRequest(const std::vector<QString> &addresses);

void parseBalancesResponseWithHandler(const QString &response, const std::function<void(const BalanceInfo &info)> &handler);

std::vector<BalanceInfo> parseBalancesResponse(const QString &response);

std::map<QString, BalanceInfo> parseBalancesResponseToMap(const QString &response);

QString makeGetHistoryRequest(const QString &address, bool isCnt, uint64_t fromTx, uint64_t cnt);

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

QString makeAddressGetTokensRequest(const QString &address);
std::vector<TokenBalance> parseAddressGetTokensResponse(const QString& address, const QString& response);

QString makeTokenGetInfoRequest(const QString& tokenAddress);
Token parseTokenGetInfoResponse(const QString& tokenAddress, const QString& response);

} // namespace transactions

#endif // TRANSACTIONSMESSAGES_H
