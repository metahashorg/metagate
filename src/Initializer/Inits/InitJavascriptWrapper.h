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
class Transactions;
}

namespace auth {
class Auth;
}

namespace utils {
class Utils;
}

namespace wallets {
class Wallets;
}

namespace metagate {
class MetaGate;
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
        SharedFuture<MainWindow> mainWindow,
        SharedFuture<transactions::Transactions> transactions,
        SharedFuture<auth::Auth> auth,
        SharedFuture<utils::Utils> utils,
        SharedFuture<wallets::Wallets> wallets,
        SharedFuture<metagate::MetaGate> metagate,
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
