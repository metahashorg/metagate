#ifndef INIT_JAVASCRIPT_WRAPPER_H
#define INIT_JAVASCRIPT_WRAPPER_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>

struct TypedException;

class JavascriptWrapper;
class MainWindow;
class NsLookup;
class WebSocketClient;

namespace transactions {
class TransactionsJavascript;
class Transactions;
}

namespace auth {
class Auth;
class AuthJavascript;
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

    void complete() override;

    Return initialize(
        std::shared_future<WebSocketClient*> wssClient,
        std::shared_future<NsLookup*> nsLookup,
        std::shared_future<MainWindow*> mainWindow,
        std::shared_future<std::pair<transactions::TransactionsJavascript*, transactions::Transactions*>> transactions,
        std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth,
        const QString &versionString
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

    void callbackCall(const Callback &callback);

private slots:

    void onCallbackCall(const Callback &callback);

private:

    std::unique_ptr<JavascriptWrapper> jsWrapper;

    bool isInitSuccess = false;

};

}

#endif // INIT_JAVASCRIPT_WRAPPER_H
