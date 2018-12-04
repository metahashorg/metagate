#include "InitAuth.h"

#include "../Initializer.h"

#include "auth/AuthJavascript.h"
#include "auth/Auth.h"
#include "mainwindow.h"

#include "check.h"

namespace initializer {

InitAuth::InitAuth(QThread *mainThread, Initializer &manager, int fromNumber, int toNumber)
    : InitInterface(mainThread, manager, fromNumber, toNumber)
{}

InitAuth::~InitAuth() = default;

void InitAuth::complete() {
    CHECK(authManager != nullptr, "authManager not initialized");
    CHECK(authJavascript != nullptr, "authJavascript not initialized");
}

void InitAuth::sendInitSuccess() {
    sendState(InitState(fromNumber, "auth", "init", "auth initialized", TypedException()));
}

InitAuth::Return InitAuth::initialize(std::shared_future<std::reference_wrapper<MainWindow>> mainWindow) {
    authJavascript = std::make_unique<auth::AuthJavascript>(mainThread);
    authManager = std::make_unique<auth::Auth>(*authJavascript);
    authManager->start();
    MainWindow &mw = mainWindow.get();
    mw.setAuthJavascript(*authJavascript);
    mw.setAuth(*authManager);

    sendInitSuccess();
    return std::make_pair(std::ref(*authManager), std::ref(*authJavascript));
}

}
