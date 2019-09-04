#ifndef PROXYCLIENTMESSAGE_H
#define PROXYCLIENTMESSAGE_H

#include <QString>

namespace proxy_client {

struct RefreshConfigResponse {
    bool isError = false;
    QString error;
};

QString makeGetStatusMessage();

QString makeRefreshConfigMessage();

RefreshConfigResponse parseRefreshConfigMessage(const std::string &message);

} // namespace proxy_client

#endif // PROXYCLIENTMESSAGE_H
