#ifndef INIT_METAGATE_H
#define INIT_METAGATE_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

struct TypedException;

class MainWindow;

class NsLookup;
class InfrastructureNsLookup;
class WebSocketClient;
class NetwrokTesting;

namespace auth {
class Auth;
class AuthJavascript;
}

namespace wallets {
class Wallets;
class WalletsJavascript;
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
        std::shared_future<WebSocketClient*> wssClient,
        std::shared_future<std::pair<NsLookup*, InfrastructureNsLookup*>> nsLookup,
        std::shared_future<MainWindow*> mainWindow,
        std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth,
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

    void callbackCall(const InitMetaGate::Callback &callback);

private slots:

    void onCallbackCall(const InitMetaGate::Callback &callback);

private:

    std::unique_ptr<metagate::MetaGate> managerMetagate;
    std::unique_ptr<metagate::MetaGateJavascript> javascript;

};

}

#endif // INIT_METAGATE_H
