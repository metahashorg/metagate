#ifndef INIT_AUTH_H
#define INIT_AUTH_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>

struct TypedException;

namespace auth {
class Auth;
class AuthJavascript;
}

class MainWindow;

namespace initializer {

class InitializerJavascript;

class InitAuth: public QObject, public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<std::reference_wrapper<auth::Auth>, std::reference_wrapper<auth::AuthJavascript>>;

    using Callback = std::function<void()>;

public:

    InitAuth(QThread *mainThread, Initializer &manager);

    ~InitAuth() override;

    void complete() override;

    Return initialize(std::shared_future<std::reference_wrapper<MainWindow>> mainWindow);

    static int countEvents() {
        return 1;
    }

private:

    void sendInitSuccess(const TypedException &exception);

signals:

    void callbackCall(const Callback &callback);

private slots:

    void onCallbackCall(const Callback &callback);

private:

    std::unique_ptr<auth::Auth> authManager;
    std::unique_ptr<auth::AuthJavascript> authJavascript;

};

}

#endif // INIT_AUTH_H
