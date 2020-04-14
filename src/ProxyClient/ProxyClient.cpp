#include "ProxyClient.h"

#include "ProxyClientMessage.h"

#include "MetaGate/MetaGate.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "utilites/machine_uid.h"

#include "Paths.h"

#include <QSettings>

SET_LOG_NAMESPACE("PXC");

namespace proxy_client {

ProxyClient::ProxyClient(metagate::MetaGate &metagate, QObject *parent)
    : TimerClass(20s, parent)
    , proxyClient(new LocalClient(getLocalServerPath(), this))
    , mhProxyActive(false)
{
    hardwareId = QString::fromStdString(::getMachineUid());

    Q_CONNECT(proxyClient, &LocalClient::callbackCall, this, &ProxyClient::callbackCall);

    Q_CONNECT(this, &ProxyClient::getStatus, this, &ProxyClient::onGetStatus);
    Q_CONNECT(this, &ProxyClient::getEnabledSetting, this, &ProxyClient::onGetEnabledSetting);
    Q_CONNECT(this, &ProxyClient::setProxyConfigAndRestart, this, &ProxyClient::onSetProxyConfigAndRestart);
    Q_CONNECT(this, &ProxyClient::getMHProxyStatus, this, &ProxyClient::onGetMHProxyStatus);

    Q_CONNECT(&metagate, &metagate::MetaGate::forgingActiveChanged, this, &ProxyClient::onForgingActiveChanged);

    Q_REG(ProxyClient::GetStatusCallback, "ProxyClient::GetStatusCallback");
    Q_REG(SetProxyConfigAndRestartCallback, "SetProxyConfigAndRestartCallback");
    Q_REG(GetEnabledSettingCallback, "GetEnabledSettingCallback");
    Q_REG(GetMHProxyStatusCallback, "GetMHProxyStatusCallback");

    moveToThread(TimerClass::getThread());

    emit metagate.activeForgingreEmit();
}

ProxyClient::~ProxyClient()
{
    TimerClass::exit();
}

//void ProxyClient::mvToThread(QThread *th) {
//    proxyClient.mvToThread(th);
//    this->moveToThread(th);
//}

void ProxyClient::startMethod()
{
    checkServiceState();
}

void ProxyClient::timerMethod()
{
    checkServiceState();
}

void ProxyClient::finishMethod()
{
}

void ProxyClient::onGetStatus(const GetStatusCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        proxyClient->sendRequest(makeGetStatusMessage(), [callback](const LocalClient::Response &response) {
            BEGIN_SLOT_WRAPPER
            QString status;
            const TypedException exception = apiVrapper2([&] {
                CHECK_TYPED(!response.exception.isSet(), TypeErrors::PROXY_SERVER_ERROR, response.exception.toString());
                const ProxyResponse result = parseProxyResponse(response.response);
                CHECK_TYPED(!result.error, TypeErrors::PROXY_RESTART_ERROR, result.text.toStdString());
                if (!response.exception.isSet()) {
                    QString hwid;
                    bool active;
                    status = parseProxyStatusResponse(response.response, hwid, active);
                }
            });
            callback.emitFunc(exception, status);
            END_SLOT_WRAPPER
        });
    }, callback);
END_SLOT_WRAPPER
}

void ProxyClient::onSetProxyConfigAndRestart(bool enabled, int port, const SetProxyConfigAndRestartCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        generateProxyConfig(enabled, port);
        proxyClient->sendRequest(makeRefreshConfigMessage(), [callback](const LocalClient::Response &response) {
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

void ProxyClient::onGetMHProxyStatus(const ProxyClient::GetMHProxyStatusCallback &callback)
{
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        return mhProxyActive;
    }, callback);
END_SLOT_WRAPPER
}

void ProxyClient::onForgingActiveChanged(bool active)
{
BEGIN_SLOT_WRAPPER
    if (active == isProxyEnabled())
        return;
    generateProxyConfig(active, 12000);
    proxyClient->sendRequest(makeRefreshConfigMessage(), [](const LocalClient::Response &response) {
                CHECK_TYPED(!response.exception.isSet(), TypeErrors::PROXY_SERVER_ERROR, response.exception.toString());
                const ProxyResponse result = parseProxyResponse(response.response);
                CHECK_TYPED(!result.error, TypeErrors::PROXY_RESTART_ERROR, result.text.toStdString());
    });
END_SLOT_WRAPPER
}

void ProxyClient::generateProxyConfig(bool enabled, int port)
{
    QSettings settings(getProxyConfigPath(), QSettings::IniFormat);
    settings.setValue("proxy/enabled", enabled);
    settings.setValue("proxy/port", port);
    settings.sync();
}

bool ProxyClient::isProxyEnabled() const
{
    QSettings settings(getProxyConfigPath(), QSettings::IniFormat);
    return settings.value("proxy/enabled", false).toBool();
}


void ProxyClient::checkServiceState()
{
BEGIN_SLOT_WRAPPER
    proxyClient->sendRequest(makeGetStatusMessage(), [this](const LocalClient::Response &response) {
        if (response.exception.isSet()) {
            mhProxyActive = false;
        } else {
            QString status;
            QString hwid;
            bool active = mhProxyActive;
            status = parseProxyStatusResponse(response.response, hwid, mhProxyActive);
            if (hwid != hardwareId) {
                LOG << "HW ids are not same: " << hardwareId << " - " << hwid;
            }
            if (active != mhProxyActive) {
                LOG << "Proxy status changed: " << mhProxyActive;
            }
        }
    });
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

