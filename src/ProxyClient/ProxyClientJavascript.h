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

    Q_INVOKABLE void refreshStatus(const QString &callback);

private:

    ProxyClient &proxyClient;
};

} // namespace proxy_client

#endif // PROXYCLIENTJAVASCRIPT_H
