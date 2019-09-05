#ifndef PROXYCLIENTMESSAGE_H
#define PROXYCLIENTMESSAGE_H

#include <QString>

#include "ProxyStatus.h"

namespace proxy_client {

struct RefreshConfigResponse {
    bool isError = false;
    QString error;
};

QString makeGetStatusMessage();

QString makeRefreshConfigMessage();

ProxyStatus parseStatusMessage(const std::string &message);

RefreshConfigResponse parseRefreshConfigMessage(const std::string &message);

} // namespace proxy_client

#endif // PROXYCLIENTMESSAGE_H
