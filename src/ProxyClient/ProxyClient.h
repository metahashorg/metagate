#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "Network/LocalClient.h"

#include "ProxyStatus.h"

namespace proxy_client {

class ProxyClient: public ManagerWrapper {
    Q_OBJECT
public:

    using RefreshStatusCallback = CallbackWrapper<void(const ProxyStatus &result)>;

public:

    ProxyClient();

    void mvToThread(QThread *th);

signals:

    void refreshStatus(const RefreshStatusCallback &callback);

private slots:

    void onRefreshStatus(const RefreshStatusCallback &callback);

private:

    LocalClient proxyClient;

};

} // namespace proxy_client

#endif // PROXYCLIENT_H
