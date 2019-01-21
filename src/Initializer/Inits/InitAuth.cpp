#include "InitAuth.h"

#include "../Initializer.h"

#include <QTimer>

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
    : InitInterface(mainThread, manager, true)
{
    CHECK(connect(this, &InitAuth::callbackCall, this, &InitAuth::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &InitAuth::checkTokenFinished, this, &InitAuth::onCheckTokenFinished), "not connect onCheckTokenFinished");
    qRegisterMetaType<Callback>("Callback");

    setTimerEvent(10s, "auth checked timeout", std::bind(&InitAuth::checkTokenFinished, this, _1));
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
    CHECK(isCheckTokenFinished, "token not checked");
    CHECK(isInitSuccess, "initialize not success");
}

void InitAuth::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("auth", "init", "auth initialized", true, exception));
    isInitSuccess = !exception.isSet();
}

void InitAuth::sendLoginCheckedSuccess(const TypedException &exception) {
    sendState(InitState("auth", "checked", "auth checked", false, exception));
}

void InitAuth::onCheckTokenFinished(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    if (!isCheckTokenFinished) {
        sendLoginCheckedSuccess(exception);
        isCheckTokenFinished = true;
    }
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
        emit mw.setAuth(authJavascript.get(), authManager.get(), std::bind(&InitAuth::callbackCall, this, _1), [this](const TypedException &e) {
            sendInitSuccess(e);
        });
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(authManager.get(), authJavascript.get());
}

}
