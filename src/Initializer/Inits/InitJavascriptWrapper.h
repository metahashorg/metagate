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

class InitJavascriptWrapper: public QObject, public InitInterface {
    Q_OBJECT
public:

    using Return = std::reference_wrapper<JavascriptWrapper>;

    using Callback = std::function<void()>;

public:

    InitJavascriptWrapper(QThread *mainThread, Initializer &manager);

    ~InitJavascriptWrapper() override;

    void complete() override;

    Return initialize(
        std::shared_future<std::reference_wrapper<WebSocketClient>> wssClient,
        std::shared_future<std::reference_wrapper<NsLookup>> nsLookup,
        std::shared_future<std::reference_wrapper<MainWindow>> mainWindow,
        std::shared_future<std::pair<std::reference_wrapper<transactions::TransactionsJavascript>, std::reference_wrapper<transactions::Transactions>>> transactions,
        std::shared_future<std::pair<std::reference_wrapper<auth::Auth>, std::reference_wrapper<auth::AuthJavascript>>> auth,
        const QString &versionString
    );

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

    std::unique_ptr<JavascriptWrapper> jsWrapper;

};

}

#endif // INIT_JAVASCRIPT_WRAPPER_H
