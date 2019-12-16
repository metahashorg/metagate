#ifndef PROXYCLIENTMESSAGE_H
#define PROXYCLIENTMESSAGE_H

#include <QString>

#include "ProxyStatus.h"

namespace proxy_client {

struct ProxyResponse {
    bool error = false;
    QString text;
};

QByteArray makeGetStatusMessage();

QByteArray makeRefreshConfigMessage();

ProxyResponse parseProxyResponse(const QByteArray &message);

QString parseProxyStatusResponse(const QByteArray &message, QString &hardwareId, bool &mhProxyActive);

} // namespace proxy_client

#endif // PROXYCLIENTMESSAGE_H
