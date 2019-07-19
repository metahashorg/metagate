#include "InitWalletsNames.h"

#include <functional>
using namespace std::placeholders;

#include "WalletNames/WalletNamesJavascript.h"
#include "WalletNames/WalletNames.h"
#include "WalletNames/WalletNamesDbStorage.h"

#include "mainwindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitWalletsNames::stateName() {
    return "wallet_names";
}

InitWalletsNames::InitWalletsNames(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, false)
{
    Q_CONNECT(this, &InitWalletsNames::callbackCall, this, &InitWalletsNames::onCallbackCall);
    Q_REG(InitWalletsNames::Callback, "InitWalletsNames::Callback");

    registerStateType("init", "wallet names initialized", true, true);
}

InitWalletsNames::~InitWalletsNames() = default;

void InitWalletsNames::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitWalletsNames::completeImpl() {
    CHECK(javascript != nullptr, "javascript not initialized");
    CHECK(manager != nullptr, "manager not initialized");
    CHECK(database != nullptr, "database not initialized");
}

void InitWalletsNames::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

InitWalletsNames::Return InitWalletsNames::initialize(std::shared_future<MainWindow*> mainWindow, std::shared_future<JavascriptWrapper*> jsWrap, std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth, std::shared_future<WebSocketClient*> wssClient, std::shared_future<std::pair<wallets::Wallets*, wallets::WalletsJavascript*>> wallets) {
    const TypedException exception = apiVrapper2([&, this] {
        database = std::make_unique<wallet_names::WalletNamesDbStorage>(getDbPath());
        database->init();

        manager = std::make_unique<wallet_names::WalletNames>(*database, *(jsWrap.get()), *auth.get().first, *wssClient.get(), *wallets.get().first);
        manager->start();

        javascript = std::make_unique<wallet_names::WalletNamesJavascript>(*manager);
        javascript->moveToThread(mainThread);

        MainWindow &mw = *mainWindow.get();
        emit mw.setWalletNamesJavascript(javascript.get(), MainWindow::SetWalletNamesJavascriptCallback([this]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitWalletsNames::sendInitSuccess, this, _1), std::bind(&InitWalletsNames::callbackCall, this, _1)));
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(javascript.get(), manager.get());
}

}
