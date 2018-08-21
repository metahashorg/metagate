#ifndef TRANSACTIONSMESSAGES_H
#define TRANSACTIONSMESSAGES_H

#include <QString>

#include <vector>

#include "Transaction.h"

namespace transactions {

QString makeGetBalanceRequest(const QString &address);

BalanceResponse parseBalanceResponse(const QString &response);

QString makeGetHistoryRequest(const QString &address, bool isCnt, uint64_t cnt);

std::vector<Transaction> parseHistoryResponse(const QString &address, const QString &response);

}

#endif // TRANSACTIONSMESSAGES_H
