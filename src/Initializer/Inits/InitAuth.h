#ifndef INIT_AUTH_H
#define INIT_AUTH_H

#include "../InitInterface.h"

#include <memory>
#include <future>

namespace auth {
class Auth;
class AuthJavascript;
}

class MainWindow;

namespace initializer {

class InitializerJavascript;

class InitAuth: public InitInterface {
public:

    InitAuth(QThread *mainThread, Initializer &manager, int fromNumber, int toNumber);

    ~InitAuth() override;

    void complete() override;

    std::pair<std::reference_wrapper<auth::Auth>, std::reference_wrapper<auth::AuthJavascript>> initialize(
        std::shared_future<std::reference_wrapper<MainWindow>> mainWindow
    );

    static int countEvents() {
        return 1;
    }

private:

    void sendInitSuccess();

private:

    std::unique_ptr<auth::Auth> authManager;
    std::unique_ptr<auth::AuthJavascript> authJavascript;

};

}

#endif // INIT_AUTH_H
