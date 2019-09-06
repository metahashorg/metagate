#ifndef PROXYCLIENTJAVASCRIPT_H
#define PROXYCLIENTJAVASCRIPT_H

#include "qt_utilites/WrapperJavascript.h"

namespace proxy_client {

class ProxyClient;

class ProxyClientJavascript: public WrapperJavascript {
    Q_OBJECT
public:

    ProxyClientJavascript(ProxyClient &proxyClient);

public:

    Q_INVOKABLE void getStatus(const QString &callback);

    Q_INVOKABLE void getEnabledSetting(const QString &callback);

    Q_INVOKABLE void setProxyConfigAndRestart(bool enabled, int port, const QString &callback);

private:

    ProxyClient &proxyClient;
};

} // namespace proxy_client

#endif // PROXYCLIENTJAVASCRIPT_H
