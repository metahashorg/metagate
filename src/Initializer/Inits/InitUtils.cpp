#include "InitUtils.h"

#include <functional>
using namespace std::placeholders;

#include "Utils/UtilsManager.h"
#include "Utils/UtilsJavascript.h"
#include "mainwindow.h"

#include "Paths.h"
#include "check.h"
#include "TypedException.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitUtils::stateName() {
    return "utils";
}

InitUtils::InitUtils(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, false)
{
    Q_CONNECT(this, &InitUtils::callbackCall, this, &InitUtils::onCallbackCall);
    Q_REG(InitUtils::Callback, "InitUtils::Callback");

    registerStateType("init", "utils initialized", true, true);
}

InitUtils::~InitUtils() = default;

void InitUtils::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void InitUtils::completeImpl() {
    CHECK(utilsManager != nullptr, "manager not initialized");
    CHECK(utilsJavascript != nullptr, "utilsJavascript not initialized");
}

void InitUtils::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

InitUtils::Return InitUtils::initialize(SharedFuture<MainWindow> mainWindow) {
    const TypedException exception = apiVrapper2([&, this] {
        utilsManager = std::make_unique<utils::Utils>();
        utilsManager->mvToThread(mainThread);
        utilsJavascript = std::make_unique<utils::UtilsJavascript>(*utilsManager);
        utilsJavascript->moveToThread(mainThread);
        MainWindow &mw = mainWindow.get();
        utilsManager->setWidget(&mw);
        emit mw.setUtilsJavascript(utilsJavascript.get(), MainWindow::SetUtilsJavascriptCallback([this]() {
            sendInitSuccess(TypedException());
        }, std::bind(&InitUtils::sendInitSuccess, this, _1), std::bind(&InitUtils::callbackCall, this, _1)));
    });

    if (exception.isSet()) {
        sendInitSuccess(exception);
        throw exception;
    }
    return std::make_pair(utilsManager.get(), utilsJavascript.get());
}

}
