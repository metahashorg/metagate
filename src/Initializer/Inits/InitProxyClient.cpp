#include "InitProxyClient.h"

#include <functional>
using namespace std::placeholders;

#include "ProxyClient/ProxyClient.h"
#include "ProxyClient/ProxyClientJavascript.h"

#include "MainWindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitProxyClient::stateName() {
    return "proxy";
}

InitProxyClient::InitProxyClient(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, false)
{
    Q_CONNECT(this, &InitProxyClient::callbackCall, this, &InitProxyClient::onCallbackCall);
    Q_REG(InitProxyClient::Callback, "InitProxyClient::Callback");

    registerStateType("init", "proxy initialized", true, true);
}

InitProxyClient::~InitProxyClient() = default;

void InitProxyClient::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitProxyClient::completeImpl() {
    CHECK(managerProxy != nullptr, "manager not initialized");
    CHECK(javascript != nullptr, "javascript not initialized");
}

void InitProxyClient::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

InitProxyClient::Return InitProxyClient::initialize(
    SharedFuture<MainWindow> mainWindow, SharedFuture<metagate::MetaGate> metagate
) {
    const TypedException exception = apiVrapper2([&, this] {
        managerProxy = std::make_unique<proxy_client::ProxyClient>(metagate.get());
        managerProxy->start();

        //managerProxy->moveToThread(mainThread);
        javascript = std::make_unique<proxy_client::ProxyClientJavascript>(*managerProxy);
        javascript->moveToThread(mainThread);
        MainWindow &mw = mainWindow.get();
        emit mw.setProxyJavascript(javascript.get(), MainWindow::SetProxyJavascriptCallback([this]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitProxyClient::sendInitSuccess, this, _1), std::bind(&InitProxyClient::callbackCall, this, _1)));
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(managerProxy.get(), javascript.get());
}

}
