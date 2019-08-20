#ifndef INIT_JAVASCRIPT_WRAPPER_H
#define INIT_JAVASCRIPT_WRAPPER_H

#include "../InitInterface.h"

#include "../SharedFuture.h"

#include <QObject>

#include <memory>
#include <future>

struct TypedException;

class JavascriptWrapper;
class MainWindow;
class NsLookup;
class WebSocketClient;
class NetwrokTesting;

namespace transactions {
class TransactionsJavascript;
class Transactions;
}

namespace auth {
class Auth;
class AuthJavascript;
}

namespace utils {
class Utils;
class UtilsJavascript;
}

namespace wallets {
class Wallets;
class WalletsJavascript;
}

namespace initializer {

class InitializerJavascript;

class InitJavascriptWrapper: public InitInterface {
    Q_OBJECT
public:

    using Return = JavascriptWrapper*;

    using Callback = std::function<void()>;

public:

    InitJavascriptWrapper(QThread *mainThread, Initializer &manager);

    ~InitJavascriptWrapper() override;

    void completeImpl() override;

    Return initialize(
        SharedFuture<WebSocketClient> wssClient,
        SharedFuture<NsLookup> nsLookup,
        std::shared_future<MainWindow*> mainWindow,
        std::shared_future<std::pair<transactions::TransactionsJavascript*, transactions::Transactions*>> transactions,
        std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth,
        std::shared_future<std::pair<utils::Utils*, utils::UtilsJavascript*>> utils,
        std::shared_future<std::pair<wallets::Wallets*, wallets::WalletsJavascript*>> wallets,
        const QString &versionString,
        NetwrokTesting &nettest
    );

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

    void callbackCall(const InitJavascriptWrapper::Callback &callback);

private slots:

    void onCallbackCall(const InitJavascriptWrapper::Callback &callback);

private:

    std::unique_ptr<JavascriptWrapper> jsWrapper;

};

}

#endif // INIT_JAVASCRIPT_WRAPPER_H
