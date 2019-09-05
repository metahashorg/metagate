#include "ProxyClient.h"

#include "ProxyClientMessage.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "Paths.h"

#include <QSettings>

SET_LOG_NAMESPACE("PXC");

namespace proxy_client {

ProxyClient::ProxyClient()
    : proxyClient("metagate_proxy")
{
    Q_CONNECT(&proxyClient, &LocalClient::callbackCall, this, &ProxyClient::callbackCall);

    Q_CONNECT(this, &ProxyClient::refreshStatus, this, &ProxyClient::onRefreshStatus);
    Q_CONNECT(this, &ProxyClient::getEnabledSetting, this, &ProxyClient::onGetEnabledSetting);
    Q_CONNECT(this, &ProxyClient::changeEnabledSetting, this, &ProxyClient::onCangeEnabledSetting);

    Q_REG(RefreshStatusCallback, "RefreshStatusCallback");
    Q_REG(ChangeEnabledSettingCallback, "ChangeEnabledSettingCallback");
    Q_REG(ChangeEnabledSettingCallback, "GetEnabledSettingCallback");
}

void ProxyClient::mvToThread(QThread *th) {
    proxyClient.mvToThread(th);
    this->moveToThread(th);
}

void ProxyClient::onRefreshStatus(const RefreshStatusCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        proxyClient.sendMessage(makeGetStatusMessage().toStdString(), [callback](const LocalClient::Response &response) {
            ProxyStatus status;
            const TypedException exception = apiVrapper2([&] {
                if (response.exception.isSet()) {
                    status.status = ProxyStatus::Status::connect_to_server_error;
                    status.description = QString::fromStdString(response.exception.description);
                } else {
                    status = parseStatusMessage(response.response);
                }
            });
            callback.emitFunc(exception, status);
        });
    }, callback);
END_SLOT_WRAPPER
}

void ProxyClient::onCangeEnabledSetting(bool enabled, const ChangeEnabledSettingCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        QSettings settings(getProxyConfigPath(), QSettings::IniFormat);
        settings.setValue("proxy/enabled", enabled);
        proxyClient.sendMessage(makeRefreshConfigMessage().toStdString(), [callback](const LocalClient::Response &response) {
            const TypedException exception = apiVrapper2([&] {
                CHECK_TYPED(!response.exception.isSet(), TypeErrors::PROXY_SERVER_ERROR, response.exception.toString());
                const RefreshConfigResponse result = parseRefreshConfigMessage(response.response);
                CHECK_TYPED(!result.isError, TypeErrors::PROXY_SERVER_NOT_REFRESH_CONFIG, result.error.toStdString());
            });
            callback.emitFunc(exception);
        });
    }, callback);
END_SLOT_WRAPPER
}

void ProxyClient::onGetEnabledSetting(const GetEnabledSettingCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        QSettings settings(getProxyConfigPath(), QSettings::IniFormat);
        return settings.value("proxy/enabled", false).toBool();
    }, callback);
END_SLOT_WRAPPER
}

} // namespace proxy_client
