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

    using GetEnabledSettingCallback = CallbackWrapper<void(bool enabled)>;

    using ChangeEnabledSettingCallback = CallbackWrapper<void()>;

public:

    ProxyClient();

    void mvToThread(QThread *th);

signals:

    void refreshStatus(const RefreshStatusCallback &callback);

    void getEnabledSetting(const GetEnabledSettingCallback &callback);

    void changeEnabledSetting(bool enabled, const ChangeEnabledSettingCallback &callback);

private slots:

    void onRefreshStatus(const RefreshStatusCallback &callback);

    void onGetEnabledSetting(const GetEnabledSettingCallback &callback);

    void onCangeEnabledSetting(bool enabled, const ChangeEnabledSettingCallback &callback);

private:

    LocalClient proxyClient;

};

} // namespace proxy_client

#endif // PROXYCLIENT_H
