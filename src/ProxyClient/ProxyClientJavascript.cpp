#include "ProxyClientJavascript.h"

#include "ProxyClient.h"

#include "Log.h"
#include "check.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/WrapperJavascriptImpl.h"

#include <QJsonObject>
#include <QJsonArray>

SET_LOG_NAMESPACE("PXC");

namespace proxy_client {

ProxyClientJavascript::ProxyClientJavascript(ProxyClient &proxyClient)
    : WrapperJavascript(false, LOG_FILE)
    , proxyClient(proxyClient)
{

}

void ProxyClientJavascript::getStatus(const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<QString>(QString()));

    wrapOperation([&, this](){
        emit proxyClient.getStatus(ProxyClient::GetStatusCallback([makeFunc](QString status){
            LOG << "Status proxy: " << status;
            makeFunc.func(TypedException(), status);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void ProxyClientJavascript::getEnabledSetting(const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback, JsTypeReturn<bool>(false));

    wrapOperation([&, this](){
        emit proxyClient.getEnabledSetting(ProxyClient::GetEnabledSettingCallback([makeFunc](bool enabled){
            LOG << "Enabled proxy: " << enabled;
            makeFunc.func(TypedException(), enabled);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void ProxyClientJavascript::setProxyConfigAndRestart(bool enabled, int port, const QString &callback) {
BEGIN_SLOT_WRAPPER
    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(callback);

    LOG << "Set proxy config: " << enabled << port;

    wrapOperation([&, this](){
        emit proxyClient.setProxyConfigAndRestart(enabled, port, ProxyClient::SetProxyConfigAndRestartCallback([makeFunc](){
            makeFunc.func(TypedException());
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

} // namespace proxy_client
