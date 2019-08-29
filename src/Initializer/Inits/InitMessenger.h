#ifndef INIT_MESSENGER_H
#define INIT_MESSENGER_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

#include "../SharedFuture.h"

struct TypedException;

namespace messenger {
class CryptographicManager;
class MessengerJavascript;
class MessengerDBStorage;
class Messenger;
}

namespace auth {
class Auth;
}

namespace transactions {
class Transactions;
}

namespace wallets {
class Wallets;
}

class MainWindow;

namespace initializer {

class InitializerJavascript;

class InitMessenger: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<messenger::MessengerJavascript*, messenger::Messenger*>;

    using Callback = std::function<void()>;

public:

    InitMessenger(QThread *mainThread, Initializer &manager);

    ~InitMessenger() override;

    void completeImpl() override;

    Return initialize(SharedFuture<MainWindow> mainWindow, SharedFuture<auth::Auth> auth, SharedFuture<transactions::Transactions> trancactions, SharedFuture<wallets::Wallets> wallets);

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

    void callbackCall(const InitMessenger::Callback &callback);

private slots:

    void onCallbackCall(const InitMessenger::Callback &callback);

private:

    std::unique_ptr<messenger::MessengerDBStorage> database;
    std::unique_ptr<messenger::MessengerJavascript> javascript;
    std::unique_ptr<messenger::CryptographicManager> crypto;
    std::unique_ptr<messenger::Messenger> manager;

};

}

#endif // INIT_MESSENGER_H
