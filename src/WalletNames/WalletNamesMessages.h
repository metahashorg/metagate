#ifndef WALLETNAMESMESSAGES_H
#define WALLETNAMESMESSAGES_H

#include <QString>
#include <QJsonDocument>

#include "WalletInfo.h"

namespace wallet_names {

QString makeRenameMessage(const QString &address, const QString &name, size_t id, const QString &token, const QString &hwid);

QString makeSetWalletsMessage(const std::vector<WalletInfo> &infos, size_t id, const QString &token, const QString &hwid);

QString makeGetWalletsMessage(size_t id, const QString &token, const QString &hwid);

enum METHOD: int {
    RENAME = 0, SET_WALLETS = 1, GET_WALLETS = 2,
    NOT_SET = 1000
};

struct ResponseType {
    METHOD method = METHOD::NOT_SET;

    enum class ERROR_TYPE {
        NO_ERROR, OTHER
    };

    bool isError = false;
    QString error;
    ERROR_TYPE errorType = ERROR_TYPE::NO_ERROR;
    size_t id = -1;
};

ResponseType getMethodAndAddressResponse(const QJsonDocument &response);

std::vector<WalletInfo> parseGetWalletsMessage(const QJsonDocument &response);

} // namespace wallet_names

#endif // WALLETNAMESMESSAGES_H
