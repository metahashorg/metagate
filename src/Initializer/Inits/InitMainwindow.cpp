#include "InitMainwindow.h"

#include "MainWindow.h"

#include "MhPayEventHandler.h"

#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitMainWindow::stateName() {
    return "window";
}

InitMainWindow::InitMainWindow(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, false)
{
    registerStateType("init", "window initialized", true, true);
}

InitMainWindow::~InitMainWindow() = default;

void InitMainWindow::completeImpl() {
    CHECK(mainWindow != nullptr, "window not initialized");
    emit mainWindow->initFinished();
}

void InitMainWindow::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

InitMainWindow::Return InitMainWindow::initialize(InitializerJavascript &initializerJs, const std::string &versionString, const std::string &typeString, const std::string &gitString, MhPayEventHandler &eventHandler, bool hide) {
    const TypedException exception = apiVrapper2([&, this] {
        mainWindow = std::make_unique<MainWindow>(initializerJs);
        mainWindow->setWindowTitle(APPLICATION_NAME + QString::fromStdString(" -- " + versionString + " " + typeString + " " + gitString));
        if (!hide)
            mainWindow->showExpanded();
        eventHandler.setMainWindow(mainWindow.get());
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return mainWindow.get();
}

}
