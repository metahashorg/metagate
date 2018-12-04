#ifndef INIT_MAINWINDOW_H
#define INIT_MAINWINDOW_H

#include "../InitInterface.h"

#include <memory>

class MainWindow;

namespace initializer {

class InitializerJavascript;

class InitMainWindow: public InitInterface {
public:

    InitMainWindow(Initializer &manager, int fromNumber, int toNumber);

    ~InitMainWindow() override;

    void complete() override;

    std::reference_wrapper<MainWindow> initialize(InitializerJavascript &initializerJs, const std::string &versionString, const std::string &typeString, const std::string &gitString);

    static int countEvents() {
        return 1;
    }

private:

    void sendInitSuccess();

private:

    std::unique_ptr<MainWindow> mainWindow;

};

}

#endif // INIT_MAINWINDOW_H
