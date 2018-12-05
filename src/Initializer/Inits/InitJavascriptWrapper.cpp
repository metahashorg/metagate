#include "InitJavascriptWrapper.h"

#include "../Initializer.h"

#include <functional>
using namespace std::placeholders;

#include "JavascriptWrapper.h"
#include "mainwindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "SlotWrapper.h"

namespace initializer {

InitJavascriptWrapper::InitJavascriptWrapper(QThread *mainThread, Initializer &manager)
    : QObject(nullptr)
    , InitInterface(mainThread, manager)
{
    CHECK(connect(this, &InitJavascriptWrapper::callbackCall, this, &InitJavascriptWrapper::onCallbackCall), "not connect onCallbackCall");
    qRegisterMetaType<Callback>("Callback");
}

InitJavascriptWrapper::~InitJavascriptWrapper() = default;

void InitJavascriptWrapper::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitJavascriptWrapper::complete() {
    CHECK(jsWrapper != nullptr, "jsWrapper not initialized");
}

void InitJavascriptWrapper::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("jsWrapper", "init", "jsWrapper initialized", exception));
}

InitJavascriptWrapper::Return InitJavascriptWrapper::initialize(
    std::shared_future<std::reference_wrapper<WebSocketClient>> wssClient,
    std::shared_future<std::reference_wrapper<NsLookup>> nsLookup,
    std::shared_future<std::reference_wrapper<MainWindow>> mainWindow,
    std::shared_future<std::pair<std::reference_wrapper<transactions::TransactionsJavascript>, std::reference_wrapper<transactions::Transactions>>> transactions,
    std::shared_future<std::pair<std::reference_wrapper<auth::Auth>, std::reference_wrapper<auth::AuthJavascript>>> auth,
    const QString &versionString
) {
    const TypedException exception = apiVrapper2([&, this] {
        jsWrapper = std::make_unique<JavascriptWrapper>(wssClient.get(), nsLookup.get(), transactions.get().second, auth.get().first, versionString);
        MainWindow &mw = mainWindow.get();
        emit mw.setJavascriptWrapper(jsWrapper.get(), std::bind(&InitJavascriptWrapper::callbackCall, this, _1), [this](const TypedException &e) {
            sendInitSuccess(e);
        });
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return *jsWrapper;
}

}
