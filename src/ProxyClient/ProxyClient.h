#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "Network/LocalClient.h"

#include "ProxyStatus.h"


namespace metagate {
class MetaGate;
}

namespace proxy_client {

class ProxyClient: public ManagerWrapper {
    Q_OBJECT
public:

    using GetStatusCallback = CallbackWrapper<void(const QString &result)>;

    using GetEnabledSettingCallback = CallbackWrapper<void(bool enabled)>;

    using SetProxyConfigAndRestartCallback = CallbackWrapper<void()>;

public:

    ProxyClient(metagate::MetaGate &metagate);

    void mvToThread(QThread *th);

signals:

    void getStatus(const GetStatusCallback &callback);

    void getEnabledSetting(const GetEnabledSettingCallback &callback);

    void setProxyConfigAndRestart(bool enabled, int port, const SetProxyConfigAndRestartCallback &callback);

private slots:

    void onGetStatus(const GetStatusCallback &callback);

    void onGetEnabledSetting(const GetEnabledSettingCallback &callback);

    void onSetProxyConfigAndRestart(bool enabled, int port, const SetProxyConfigAndRestartCallback &callback);

private slots:

    void onForgingActiveChanged(bool active);

private:
    void generateProxyConfig(bool enabled, int port);

    bool isProxyEnabled() const;

private:
    LocalClient proxyClient;

};

} // namespace proxy_client

#endif // PROXYCLIENT_H
