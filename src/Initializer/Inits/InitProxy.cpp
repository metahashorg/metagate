#include "InitProxy.h"

#include <QTimer>

#include <functional>
using namespace std::placeholders;

#include "auth/AuthJavascript.h"
#include "auth/Auth.h"
#include "mainwindow.h"

#include "check.h"
#include "TypedException.h"
#include "SlotWrapper.h"
#include "Log.h"
#include "QRegister.h"

#include "proxy/Proxy.h"
#include "proxy/ProxyJavascript.h"
#include "proxy/WebSocketSender.h"
#include "Module.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitProxy::stateName() {
    return "proxy";
}

InitProxy::InitProxy(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, true)
{
    CHECK(connect(this, &InitProxy::callbackCall, this, &InitProxy::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &InitProxy::upnpStarted, this, &InitProxy::onUpnpStarted), "not connect onUpnpStarted");
    CHECK(connect(this, &InitProxy::proxyStarted, this, &InitProxy::onProxyStarted), "not connect onProxyStarted");
    CHECK(connect(this, &InitProxy::proxyCompleted, this, &InitProxy::onProxyCompleted), "not connect onProxyCompleted");
    Q_REG(InitProxy::Callback, "InitProxy::Callback");

    registerStateType("init", "proxy initialized", true, true);
    registerStateType("upnp_started", "upnp started", false, false, 10s, "upnp started timeout");
    registerStateType("proxy_started", "proxy started", false, false, 12s, "proxy started timeout");
    registerStateType("proxy_completed", "proxy complete", false, false, 15s, "proxy complete timeout");
}

InitProxy::~InitProxy() = default;

void InitProxy::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitProxy::completeImpl() {
    CHECK(proxyManager != nullptr, "proxyManager not initialized");
    CHECK(proxyJavascript != nullptr, "proxyJavascript not initialized");
    CHECK(proxyWebsocket != nullptr, "proxyWebsocket not initialized");
}

void InitProxy::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

void InitProxy::sendUpnpStarted(bool isScipped, const TypedException &exception) {
    sendState("upnp_started", isScipped, exception);
}

void InitProxy::sendProxyStarted(bool isScipped, const TypedException &exception) {
    sendState("proxy_started", isScipped, exception);
}

void InitProxy::sendProxyCompleted(bool isScipped, const TypedException &exception) {
    sendState("proxy_completed", isScipped, exception);
}

void InitProxy::onUpnpStarted(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    sendUpnpStarted(false, exception);
END_SLOT_WRAPPER
}

void InitProxy::onProxyCompleted(bool res) {
BEGIN_SLOT_WRAPPER
    sendProxyCompleted(false, TypedException());
END_SLOT_WRAPPER
}

void InitProxy::onProxyStarted(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    sendProxyStarted(false, exception);
END_SLOT_WRAPPER
}

InitProxy::Return InitProxy::initialize(std::shared_future<MainWindow*> mainWindow) {
    const TypedException exception = apiVrapper2([&, this] {
        proxyJavascript = std::make_unique<proxy::ProxyJavascript>();
        proxyJavascript->moveToThread(mainThread);
        proxyManager = std::make_unique<proxy::Proxy>(*proxyJavascript);
        CHECK(connect(proxyManager.get(), &proxy::Proxy::startAutoProxyResult, this, &InitProxy::proxyStarted), "not connect onProxyStarted");
        CHECK(connect(proxyManager.get(), &proxy::Proxy::startAutoUPnPResult, this, &InitProxy::upnpStarted), "not connect onUpnpStarted");
        CHECK(connect(proxyManager.get(), &proxy::Proxy::startAutoComplete1, this, &InitProxy::proxyCompleted), "not connect proxyCompleted");
        proxyWebsocket = std::make_unique<proxy::WebSocketSender>(*proxyManager);
        proxyWebsocket->moveToThread(mainThread);
        changeStatus(proxy::Proxy::moduleName(), StatusModule::found);
        MainWindow &mw = *mainWindow.get();
        emit mw.setProxyJavascript(proxyJavascript.get(), MainWindow::SetProxyJavascriptCallback([this]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitProxy::sendInitSuccess, this, _1), std::bind(&InitProxy::callbackCall, this, _1)));
    });
    if (proxyManager != nullptr && !proxyManager->isAutoStart()) {
        sendUpnpStarted(true, TypedException());
        sendProxyStarted(true, TypedException());
        sendProxyCompleted(true, TypedException());
    }
    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_tuple(proxyManager.get(), proxyJavascript.get(), proxyWebsocket.get());
}

}
