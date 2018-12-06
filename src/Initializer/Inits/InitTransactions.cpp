#include "InitTransactions.h"

#include "../Initializer.h"

#include <functional>
using namespace std::placeholders;

#include "transactions/TransactionsDBStorage.h"
#include "transactions/TransactionsJavascript.h"
#include "transactions/Transactions.h"
#include "mainwindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "SlotWrapper.h"

namespace initializer {

InitTransactions::InitTransactions(QThread *mainThread, Initializer &manager)
    : QObject(nullptr)
    , InitInterface(mainThread, manager)
{
    CHECK(connect(this, &InitTransactions::callbackCall, this, &InitTransactions::onCallbackCall), "not connect onCallbackCall");
    qRegisterMetaType<Callback>("Callback");
}

InitTransactions::~InitTransactions() = default;

void InitTransactions::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitTransactions::complete() {
    CHECK(database != nullptr, "database not initialized");
    CHECK(txJavascript != nullptr, "txJavascript not initialized");
    CHECK(txManager != nullptr, "txManager not initialized");
}

void InitTransactions::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("transactions", "init", "transactions initialized", exception));
}

InitTransactions::Return InitTransactions::initialize(std::shared_future<std::reference_wrapper<MainWindow>> mainWindow, std::shared_future<std::reference_wrapper<NsLookup>> nsLookup) {
    const TypedException exception = apiVrapper2([&, this] {
        database = std::make_unique<transactions::TransactionsDBStorage>(getDbPath());
        database->init();
        txJavascript = std::make_unique<transactions::TransactionsJavascript>();
        txJavascript->moveToThread(mainThread);
        txManager = std::make_unique<transactions::Transactions>(nsLookup.get(), *txJavascript, *database);
        txManager->start();
        MainWindow &mw = mainWindow.get();
        emit mw.setTransactionsJavascript(txJavascript.get(), std::bind(&InitTransactions::callbackCall, this, _1), [this, mainWindow](const TypedException &e) {
            sendInitSuccess(e);
        });
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(std::ref(*txJavascript), std::ref(*txManager));
}

}
