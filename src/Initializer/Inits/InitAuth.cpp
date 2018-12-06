#include "InitAuth.h"

#include "../Initializer.h"

#include <functional>
using namespace std::placeholders;

#include "auth/AuthJavascript.h"
#include "auth/Auth.h"
#include "mainwindow.h"

#include "check.h"
#include "TypedException.h"
#include "SlotWrapper.h"

namespace initializer {

InitAuth::InitAuth(QThread *mainThread, Initializer &manager)
    : QObject(nullptr)
    , InitInterface(mainThread, manager)
{
    CHECK(connect(this, &InitAuth::callbackCall, this, &InitAuth::onCallbackCall), "not connect onCallbackCall");
    qRegisterMetaType<Callback>("Callback");
}

InitAuth::~InitAuth() = default;

void InitAuth::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitAuth::complete() {
    CHECK(authManager != nullptr, "authManager not initialized");
    CHECK(authJavascript != nullptr, "authJavascript not initialized");
}

void InitAuth::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("auth", "init", "auth initialized", exception));
}

InitAuth::Return InitAuth::initialize(std::shared_future<std::reference_wrapper<MainWindow>> mainWindow) {
    const TypedException exception = apiVrapper2([&, this] {
        authJavascript = std::make_unique<auth::AuthJavascript>();
        authJavascript->moveToThread(mainThread);
        authManager = std::make_unique<auth::Auth>(*authJavascript);
        authManager->start();
        MainWindow &mw = mainWindow.get();
        emit mw.setAuthJavascript(authJavascript.get(), std::bind(&InitAuth::callbackCall, this, _1), [this, mainWindow](const TypedException &e) {
            MainWindow &mw = mainWindow.get();
            if (e.isSet()) {
                sendInitSuccess(e);
                return;
            }
            emit mw.setAuth(authManager.get(), std::bind(&InitAuth::callbackCall, this, _1), [this, mainWindow](const TypedException &e) {
                sendInitSuccess(e);
            });
        });
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(std::ref(*authManager), std::ref(*authJavascript));
}

}
