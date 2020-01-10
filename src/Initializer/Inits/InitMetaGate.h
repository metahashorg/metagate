#ifndef INIT_METAGATE_H
#define INIT_METAGATE_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

#include "../SharedFuture.h"

struct TypedException;

class MainWindow;

class NsLookup;
class WebSocketClient;
class NetwrokTesting;
class Uploader;

namespace auth {
class Auth;
}

namespace wallets {
class Wallets;
}

namespace transactions {
class Transactions;
}

namespace metagate {
class MetaGate;
class MetaGateJavascript;
}

namespace initializer {

class InitializerJavascript;

class InitMetaGate: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<metagate::MetaGate*, metagate::MetaGateJavascript*>;

    using Callback = std::function<void()>;

public:

    InitMetaGate(QThread *mainThread, Initializer &manager);

    ~InitMetaGate() override;

    void completeImpl() override;

    Return initialize(
        SharedFuture<WebSocketClient> wssClient,
        SharedFuture<NsLookup> nsLookup,
        SharedFuture<MainWindow> mainWindow,
        SharedFuture<auth::Auth> auth,
        SharedFuture<wallets::Wallets> wallets,
        SharedFuture<transactions::Transactions> transactions,
        SharedFuture<Uploader> uploader,
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

    void callbackCall(const InitMetaGate::Callback &callback);

private slots:

    void onCallbackCall(const InitMetaGate::Callback &callback);

private:

    std::unique_ptr<metagate::MetaGate> managerMetagate;
    std::unique_ptr<metagate::MetaGateJavascript> javascript;

};

}

#endif // INIT_METAGATE_H
