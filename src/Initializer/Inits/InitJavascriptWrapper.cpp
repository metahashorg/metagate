#include "InitJavascriptWrapper.h"

#include <functional>
using namespace std::placeholders;

#include "JavascriptWrapper.h"
#include "MainWindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitJavascriptWrapper::stateName() {
    return "jsWrapper";
}

InitJavascriptWrapper::InitJavascriptWrapper(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, false)
{
    Q_CONNECT(this, &InitJavascriptWrapper::callbackCall, this, &InitJavascriptWrapper::onCallbackCall);
    Q_REG(InitJavascriptWrapper::Callback, "InitJavascriptWrapper::Callback");

    registerStateType("init", "jsWrapper initialized", true, true);
}

InitJavascriptWrapper::~InitJavascriptWrapper() = default;

void InitJavascriptWrapper::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitJavascriptWrapper::completeImpl() {
    CHECK(jsWrapper != nullptr, "jsWrapper not initialized");
}

void InitJavascriptWrapper::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

InitJavascriptWrapper::Return InitJavascriptWrapper::initialize(
    SharedFuture<WebSocketClient> wssClient,
    SharedFuture<NsLookup> nsLookup,
    SharedFuture<MainWindow> mainWindow,
    SharedFuture<transactions::Transactions> transactions,
    SharedFuture<auth::Auth> auth,
    SharedFuture<utils::Utils> utils,
    SharedFuture<wallets::Wallets> wallets,
    SharedFuture<metagate::MetaGate> metagate,
    const QString &versionString,
    NetwrokTesting &nettest
) {
    const TypedException exception = apiVrapper2([&, this] {
        MainWindow &mw = mainWindow.get();
        jsWrapper = std::make_unique<JavascriptWrapper>(mw, wssClient.get(), nsLookup.get(), transactions.get(), auth.get(), nettest, utils.get(), wallets.get(), metagate.get(), versionString);
        jsWrapper->mvToThread(mainThread);
        emit mw.setJavascriptWrapper(jsWrapper.get(), MainWindow::SetJavascriptWrapperCallback([this]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitJavascriptWrapper::sendInitSuccess, this, _1), std::bind(&InitJavascriptWrapper::callbackCall, this, _1)));
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return jsWrapper.get();
}

}
