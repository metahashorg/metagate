#ifndef INIT_MESSENGER_H
#define INIT_MESSENGER_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

struct TypedException;

class JavascriptWrapper;

namespace messenger {
class CryptographicManager;
class MessengerJavascript;
class MessengerDBStorage;
class Messenger;
}

namespace auth {
class Auth;
class AuthJavascript;
}

namespace transactions {
class Transactions;
class TransactionsJavascript;
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

    Return initialize(std::shared_future<MainWindow*> mainWindow, std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth, std::shared_future<std::pair<transactions::TransactionsJavascript*, transactions::Transactions*>> trancactions, std::shared_future<JavascriptWrapper*> jsWrap);

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
