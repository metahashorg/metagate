#include "InitWallets.h"

#include <functional>
using namespace std::placeholders;

#include "Wallets/Wallets.h"
#include "Wallets/WalletsJavascript.h"
#include "mainwindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitWallets::stateName() {
    return "wallets";
}

InitWallets::InitWallets(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, false)
{
    Q_CONNECT(this, &InitWallets::callbackCall, this, &InitWallets::onCallbackCall);
    Q_REG(InitWallets::Callback, "InitWallets::Callback");

    registerStateType("init", "wallets initialized", true, true);
}

InitWallets::~InitWallets() = default;

void InitWallets::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitWallets::completeImpl() {
    CHECK(manager != nullptr, "manager not initialized");
    CHECK(javascript != nullptr, "utilsJavascript not initialized");
}

void InitWallets::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

InitWallets::Return InitWallets::initialize(std::shared_future<MainWindow*> mainWindow, std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth, std::shared_future<std::pair<utils::Utils*, utils::UtilsJavascript*>> utils) {
    const TypedException exception = apiVrapper2([&, this] {
        manager = std::make_unique<wallets::Wallets>(*auth.get().first, *utils.get().first);
        manager->start();
        javascript = std::make_unique<wallets::WalletsJavascript>(*manager);
        javascript->moveToThread(mainThread);
        MainWindow &mw = *mainWindow.get();
        emit mw.setWalletsJavascript(javascript.get(), MainWindow::SetWalletsJavascriptCallback([this, mainWindow]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitWallets::sendInitSuccess, this, _1), std::bind(&InitWallets::callbackCall, this, _1)));
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(manager.get(), javascript.get());
}

}
