#include "InitMainwindow.h"

#include "../Initializer.h"

#include "mainwindow.h"

#include "check.h"

namespace initializer {

InitMainWindow::InitMainWindow(QThread *mainThread, Initializer &manager, int fromNumber, int toNumber)
    : InitInterface(mainThread, manager, fromNumber, toNumber)
{}

InitMainWindow::~InitMainWindow() = default;

void InitMainWindow::complete() {
    CHECK(mainWindow != nullptr, "mainwindow not initialized");
}

void InitMainWindow::sendInitSuccess() {
    sendState(InitState(fromNumber, "mainwindow", "init", "mainwindow initialized", TypedException()));
}

std::reference_wrapper<MainWindow> InitMainWindow::initialize(InitializerJavascript &initializerJs, const std::string &versionString, const std::string &typeString, const std::string &gitString) {
    mainWindow = std::make_unique<MainWindow>(initializerJs);
    mainWindow->setWindowTitle(APPLICATION_NAME + QString::fromStdString(" -- " + versionString + " " + typeString + " " + gitString));
    mainWindow->showExpanded();
    sendInitSuccess();
    return *mainWindow;
}

}
