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
    : proxyClient(QStringLiteral("metahash.metagate"))
{
    Q_CONNECT(&proxyClient, &LocalClient::callbackCall, this, &ProxyClient::callbackCall);

    Q_CONNECT(this, &ProxyClient::getStatus, this, &ProxyClient::onGetStatus);
    Q_CONNECT(this, &ProxyClient::getEnabledSetting, this, &ProxyClient::onGetEnabledSetting);
    Q_CONNECT(this, &ProxyClient::setProxyConfigAndRestart, this, &ProxyClient::onSetProxyConfigAndRestart);

    Q_REG(ProxyClient::GetStatusCallback, "ProxyClient::GetStatusCallback");
    Q_REG(SetProxyConfigAndRestartCallback, "SetProxyConfigAndRestartCallback");
    Q_REG(GetEnabledSettingCallback, "GetEnabledSettingCallback");
}

void ProxyClient::mvToThread(QThread *th) {
    proxyClient.mvToThread(th);
    this->moveToThread(th);
}

void ProxyClient::onGetStatus(const GetStatusCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        proxyClient.sendRequest(makeGetStatusMessage(), [callback](const LocalClient::Response &response) {
            QString status;
            const TypedException exception = apiVrapper2([&] {
                CHECK_TYPED(!response.exception.isSet(), TypeErrors::PROXY_SERVER_ERROR, response.exception.toString());
                const ProxyResponse result = parseProxyResponse(response.response);
                CHECK_TYPED(!result.error, TypeErrors::PROXY_RESTART_ERROR, result.text.toStdString());
                if (!response.exception.isSet()) {
                    status = parseProxyStatusResponse(response.response);
                }
            });
            callback.emitFunc(exception, status);
        });
    }, callback);
END_SLOT_WRAPPER
}

void ProxyClient::onSetProxyConfigAndRestart(bool enabled, int port, const SetProxyConfigAndRestartCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        generateProxyConfig(enabled, port);
        proxyClient.sendRequest(makeRefreshConfigMessage(), [callback](const LocalClient::Response &response) {
            const TypedException exception = apiVrapper2([&] {
                CHECK_TYPED(!response.exception.isSet(), TypeErrors::PROXY_SERVER_ERROR, response.exception.toString());
                const ProxyResponse result = parseProxyResponse(response.response);
                CHECK_TYPED(!result.error, TypeErrors::PROXY_RESTART_ERROR, result.text.toStdString());
            });
            callback.emitFunc(exception);
        });
    }, callback);
END_SLOT_WRAPPER
}

void ProxyClient::generateProxyConfig(bool enabled, int port)
{
    QSettings settings(getProxyConfigPath(), QSettings::IniFormat);
    settings.setValue("proxy/enabled", enabled);
    settings.setValue("proxy/port", port);
    settings.sync();
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
