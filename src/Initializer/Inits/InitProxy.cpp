#include "InitProxy.h"

#include "../Initializer.h"

#include <QTimer>

#include <functional>
using namespace std::placeholders;

#include "auth/AuthJavascript.h"
#include "auth/Auth.h"
#include "mainwindow.h"

#include "check.h"
#include "TypedException.h"
#include "SlotWrapper.h"
#include "QRegister.h"

#include "proxy/Proxy.h"
#include "proxy/ProxyJavascript.h"
#include "proxy/WebSocketSender.h"
#include "Module.h"

namespace initializer {

QString InitProxy::stateName() {
    return "proxy";
}

InitProxy::InitProxy(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, true)
{
    CHECK(connect(this, &InitProxy::callbackCall, this, &InitProxy::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &InitProxy::upnpStarted, this, &InitProxy::onUpnpStarted), "not connect onUpnpStarted");
    Q_REG(InitProxy::Callback, "InitProxy::Callback");

    registerStateType("init", "auth initialized", true, true);
    registerStateType("checked", "auth checked", false, false, 10s, "auth checked timeout");
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

void InitProxy::sendLoginCheckedSuccess(const TypedException &exception) {
    sendState("checked", false, exception);
}

void InitProxy::onUpnpStarted(bool isScipped, const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    sendLoginCheckedSuccess(exception);
END_SLOT_WRAPPER
}

InitProxy::Return InitProxy::initialize(std::shared_future<WebSocketClient*> wssClient, std::shared_future<MainWindow*> mainWindow) {
    const TypedException exception = apiVrapper2([&, this] {
        addModule(proxy::Proxy::moduleName());
        proxyJavascript = std::make_unique<proxy::ProxyJavascript>();
        proxyJavascript->moveToThread(mainThread);
        proxyManager = std::make_unique<proxy::Proxy>(*proxyJavascript);
        proxyWebsocket = std::make_unique<proxy::WebSocketSender>(*(wssClient.get()), *proxyManager);
        proxyWebsocket->moveToThread(mainThread);
        changeStatus(proxy::Proxy::moduleName(), StatusModule::found);
        MainWindow &mw = *mainWindow.get();
        emit mw.setProxyJavascript(proxyJavascript.get(), MainWindow::SetProxyJavascriptCallback([this]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitProxy::sendInitSuccess, this, _1), std::bind(&InitProxy::callbackCall, this, _1)));

        /*QObject::connect(&proxyManager, &proxy::Proxy::startAutoExecued, [](){
            qDebug() << "PROXY S ";
        });
        QObject::connect(&proxyManager, &proxy::Proxy::startAutoProxyResult, [](const TypedException &r){
            qDebug() << "PROXY 1 " << r.numError;
        });
        QObject::connect(&proxyManager, &proxy::Proxy::startAutoUPnPResult, [](const TypedException &r){
            qDebug() << "PROXY 2 " << r.numError;
        });
        QObject::connect(&proxyManager, &proxy::Proxy::startAutoComplete, [](quint16 port){
            qDebug() << "PROXY res " << port;
        });*/
    });
    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_tuple(proxyManager.get(), proxyJavascript.get(), proxyWebsocket.get());
}

}
