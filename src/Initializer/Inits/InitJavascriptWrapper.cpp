#include "InitJavascriptWrapper.h"

#include <functional>
using namespace std::placeholders;

#include "JavascriptWrapper.h"
#include "mainwindow.h"

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
    std::shared_future<WebSocketClient*> wssClient,
    std::shared_future<NsLookup*> nsLookup,
    std::shared_future<MainWindow*> mainWindow,
    std::shared_future<std::pair<transactions::TransactionsJavascript*, transactions::Transactions*>> transactions,
    std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth,
    std::shared_future<std::pair<utils::Utils*, utils::UtilsJavascript*>> utils,
    std::shared_future<std::pair<wallets::Wallets*, wallets::WalletsJavascript*>> wallets,
    const QString &versionString,
    NetwrokTesting &nettest
) {
    const TypedException exception = apiVrapper2([&, this] {
        MainWindow &mw = *mainWindow.get();
        jsWrapper = std::make_unique<JavascriptWrapper>(mw, *wssClient.get(), *nsLookup.get(), *transactions.get().second, *auth.get().first, nettest, *utils.get().first, *wallets.get().first, versionString);
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
