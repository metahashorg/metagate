#ifndef INIT_WALLET_NAMES_H
#define INIT_WALLET_NAMES_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

#include "../SharedFuture.h"

struct TypedException;

namespace wallet_names {
class WalletNamesJavascript;
class WalletNames;
class WalletNamesDbStorage;
}

namespace auth {
class Auth;
}

namespace wallets {
class Wallets;
}

class MainWindow;
class WebSocketClient;

namespace initializer {

class InitializerJavascript;

class InitWalletsNames: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<wallet_names::WalletNamesJavascript*, wallet_names::WalletNames*>;

    using Callback = std::function<void()>;

public:

    InitWalletsNames(QThread *mainThread, Initializer &manager);

    ~InitWalletsNames() override;

    void completeImpl() override;

    Return initialize(SharedFuture<MainWindow> mainWindow, SharedFuture<auth::Auth> auth, SharedFuture<WebSocketClient> wssClient, SharedFuture<wallets::Wallets> wallets);

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

    void callbackCall(const InitWalletsNames::Callback &callback);

private slots:

    void onCallbackCall(const InitWalletsNames::Callback &callback);

private:

    std::unique_ptr<wallet_names::WalletNamesJavascript> javascript;
    std::unique_ptr<wallet_names::WalletNames> manager;
    std::unique_ptr<wallet_names::WalletNamesDbStorage> database;

};

}

#endif // INIT_WALLET_NAMES_H
