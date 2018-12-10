#include "InitMainwindow.h"

#include "../Initializer.h"

#include "mainwindow.h"

#include "check.h"

namespace initializer {

InitMainWindow::InitMainWindow(QThread *mainThread, Initializer &manager)
    : InitInterface(mainThread, manager, false)
{}

InitMainWindow::~InitMainWindow() = default;

void InitMainWindow::complete() {
    CHECK(mainWindow != nullptr, "window not initialized");
    emit mainWindow->initFinished();
}

void InitMainWindow::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("window", "init", "window initialized", exception));
}

InitMainWindow::Return InitMainWindow::initialize(InitializerJavascript &initializerJs, const std::string &versionString, const std::string &typeString, const std::string &gitString) {
    const TypedException exception = apiVrapper2([&, this] {
        mainWindow = std::make_unique<MainWindow>(initializerJs);
        mainWindow->setWindowTitle(APPLICATION_NAME + QString::fromStdString(" -- " + versionString + " " + typeString + " " + gitString));
        mainWindow->showExpanded();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return mainWindow.get();
}

}
