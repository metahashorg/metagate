#ifndef INIT_MAINWINDOW_H
#define INIT_MAINWINDOW_H

#include "../InitInterface.h"

#include <memory>
#include <string>

class MainWindow;
class MhPayEventHandler;

namespace tor {
class TorProxy;
}
struct TypedException;

namespace initializer {

class InitializerJavascript;

class InitMainWindow: public InitInterface {
public:

    using Return = MainWindow*;

public:

    InitMainWindow(QThread *mainThread, Initializer &manager);

    ~InitMainWindow() override;

    void completeImpl() override;

    Return initialize(InitializerJavascript &initializerJs, tor::TorProxy &torProxy, const std::string &versionString, const std::string &typeString, const std::string &gitString, MhPayEventHandler &eventHandler, bool hide);

    static int countEvents() {
        return 1;
    }

    static int countCriticalEvents() {
        return 1;
    }

    static QString stateName();

private:

    void sendInitSuccess(const TypedException &exception);

private:

    std::unique_ptr<MainWindow> mainWindow;

};

}

#endif // INIT_MAINWINDOW_H
