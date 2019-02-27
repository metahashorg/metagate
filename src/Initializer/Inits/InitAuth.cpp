#include "InitAuth.h"

#include <functional>
using namespace std::placeholders;

#include "auth/AuthJavascript.h"
#include "auth/Auth.h"
#include "mainwindow.h"

#include "check.h"
#include "TypedException.h"
#include "SlotWrapper.h"
#include "QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitAuth::stateName() {
    return "auth";
}

InitAuth::InitAuth(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, true)
{
    CHECK(connect(this, &InitAuth::callbackCall, this, &InitAuth::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &InitAuth::checkTokenFinished, this, &InitAuth::onCheckTokenFinished), "not connect onCheckTokenFinished");
    Q_REG(InitAuth::Callback, "InitAuth::Callback");

    registerStateType("init", "auth initialized", true, true);
    registerStateType("checked", "auth checked", false, false, 10s, "auth checked timeout");
}

InitAuth::~InitAuth() = default;

void InitAuth::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitAuth::completeImpl() {
    CHECK(authManager != nullptr, "authManager not initialized");
    CHECK(authJavascript != nullptr, "authJavascript not initialized");
}

void InitAuth::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

void InitAuth::sendLoginCheckedSuccess(const TypedException &exception) {
    sendState("checked", false, exception);
}

void InitAuth::onCheckTokenFinished(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    sendLoginCheckedSuccess(exception);
END_SLOT_WRAPPER
}

InitAuth::Return InitAuth::initialize(std::shared_future<MainWindow*> mainWindow) {
    const TypedException exception = apiVrapper2([&, this] {
        authJavascript = std::make_unique<auth::AuthJavascript>();
        authJavascript->moveToThread(mainThread);
        authManager = std::make_unique<auth::Auth>(*authJavascript);
        CHECK(connect(authManager.get(), &auth::Auth::checkTokenFinished, this, &InitAuth::checkTokenFinished), "not connect checkTokenFinished");
        authManager->start();
        MainWindow &mw = *mainWindow.get();
        emit mw.setAuth(authJavascript.get(), authManager.get(), MainWindow::SetAuthCallback([this]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitAuth::sendInitSuccess, this, _1), std::bind(&InitAuth::callbackCall, this, _1)));       
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(authManager.get(), authJavascript.get());
}

}
