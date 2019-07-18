#include "InitMessenger.h"

#include <functional>
using namespace std::placeholders;

#include "Messenger/Messenger.h"
#include "Messenger/MessengerJavascript.h"
#include "Messenger/MessengerDBStorage.h"
#include "Messenger/CryptographicManager.h"

#include "mainwindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitMessenger::stateName() {
    return "messenger";
}

InitMessenger::InitMessenger(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, false)
{
    Q_CONNECT(this, &InitMessenger::callbackCall, this, &InitMessenger::onCallbackCall);
    Q_REG(InitMessenger::Callback, "InitMessenger::Callback");

    registerStateType("init", "messenger initialized", true, true);
}

InitMessenger::~InitMessenger() = default;

void InitMessenger::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitMessenger::completeImpl() {
    CHECK(database != nullptr, "database not initialized");
    CHECK(javascript != nullptr, "javascript not initialized");
    CHECK(crypto != nullptr, "crypto not initialized");
    CHECK(manager != nullptr, "manager not initialized");
}

void InitMessenger::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

InitMessenger::Return InitMessenger::initialize(std::shared_future<MainWindow*> mainWindow, std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth, std::shared_future<std::pair<transactions::TransactionsJavascript*, transactions::Transactions*>> trancactions, std::shared_future<JavascriptWrapper*> jsWrap) {
    const TypedException exception = apiVrapper2([&, this] {
        crypto = std::make_unique<messenger::CryptographicManager>();
        crypto->start();
        javascript = std::make_unique<messenger::MessengerJavascript>(*(auth.get().first), *crypto, *(trancactions.get().second), *(jsWrap.get()));
        javascript->moveToThread(mainThread);
        database = std::make_unique<messenger::MessengerDBStorage>(getDbPath());
        database->init();
        manager = std::make_unique<messenger::Messenger>(*javascript, *database, *crypto, *(mainWindow.get()));
        manager->start();
        javascript->setMessenger(*manager);

        MainWindow &mw = *mainWindow.get();
        emit mw.setMessengerJavascript(javascript.get(), MainWindow::SetMessengerJavascriptCallback([this]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitMessenger::sendInitSuccess, this, _1), std::bind(&InitMessenger::callbackCall, this, _1)));
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(javascript.get(), manager.get());
}

}
