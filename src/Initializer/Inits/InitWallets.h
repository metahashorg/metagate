#ifndef INIT_WALLETS_H
#define INIT_WALLETS_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

#include "../SharedFuture.h"

struct TypedException;

class MainWindow;

namespace wallets {
class Wallets;
class WalletsJavascript;
}

namespace auth {
class Auth;
}

namespace utils {
class Utils;
}

namespace initializer {

class InitializerJavascript;

class InitWallets: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<wallets::Wallets*, wallets::WalletsJavascript*>;

    using Callback = std::function<void()>;

public:

    InitWallets(QThread *mainThread, Initializer &manager);

    ~InitWallets() override;

    void completeImpl() override;

    Return initialize(SharedFuture<MainWindow> mainWindow, SharedFuture<auth::Auth> auth, SharedFuture<utils::Utils> utils);

    static int countEvents() {
        return 1;
    }

    static int countCriticalEvents() {
        return 1;
    }

    static QString stateName();

private:

    void sendInitSuccess(const TypedException &exception);

signals:

    void callbackCall(const InitWallets::Callback &callback);

private slots:

    void onCallbackCall(const InitWallets::Callback &callback);

private:

    std::unique_ptr<wallets::Wallets> manager;
    std::unique_ptr<wallets::WalletsJavascript> javascript;

};

}

#endif // INIT_WALLETS_H
