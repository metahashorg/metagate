#ifndef INIT_MAINWINDOW_H
#define INIT_MAINWINDOW_H

#include "../InitInterface.h"

#include <memory>
#include <string>

class MainWindow;

struct TypedException;

namespace initializer {

class InitializerJavascript;

class InitMainWindow: public InitInterface {
public:

    using Return = MainWindow*;

public:

    InitMainWindow(QThread *mainThread, Initializer &manager);

    ~InitMainWindow() override;

    void complete() override;

    Return initialize(InitializerJavascript &initializerJs, const std::string &versionString, const std::string &typeString, const std::string &gitString);

    static int countEvents() {
        return 1;
    }

private:

    void sendInitSuccess(const TypedException &exception);

private:

    std::unique_ptr<MainWindow> mainWindow;

};

}

#endif // INIT_MAINWINDOW_H
