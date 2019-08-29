#include "InitTransactions.h"

#include <functional>
using namespace std::placeholders;

#include "transactions/TransactionsDBStorage.h"
#include "transactions/TransactionsJavascript.h"
#include "transactions/Transactions.h"
#include "mainwindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitTransactions::stateName() {
    return "transactions";
}

InitTransactions::InitTransactions(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, false)
{
    Q_CONNECT(this, &InitTransactions::callbackCall, this, &InitTransactions::onCallbackCall);
    Q_REG(InitTransactions::Callback, "InitTransactions::Callback");

    registerStateType("init", "transactions initialized", true, true);
}

InitTransactions::~InitTransactions() = default;

void InitTransactions::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitTransactions::completeImpl() {
    CHECK(database != nullptr, "database not initialized");
    CHECK(txJavascript != nullptr, "txJavascript not initialized");
    CHECK(txManager != nullptr, "txManager not initialized");
}

void InitTransactions::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

InitTransactions::Return InitTransactions::initialize(SharedFuture<MainWindow> mainWindow, std::shared_future<std::pair<NsLookup*, InfrastructureNsLookup*>> nsLookup, SharedFuture<auth::Auth> auth, SharedFuture<wallets::Wallets> wallets) {
    const TypedException exception = apiVrapper2([&, this] {
        database = std::make_unique<transactions::TransactionsDBStorage>(getDbPath());
        database->init();
        txJavascript = std::make_unique<transactions::TransactionsJavascript>();
        txJavascript->moveToThread(mainThread);
        txManager = std::make_unique<transactions::Transactions>(*nsLookup.get().first, *nsLookup.get().second, *txJavascript, *database, auth.get(), mainWindow.get(), wallets.get());
        txManager->start();
        MainWindow &mw = mainWindow.get();
        emit mw.setTransactionsJavascript(txJavascript.get(), MainWindow::SetTransactionsJavascriptCallback([this, mainWindow]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitTransactions::sendInitSuccess, this, _1), std::bind(&InitTransactions::callbackCall, this, _1)));
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(txJavascript.get(), txManager.get());
}

}
