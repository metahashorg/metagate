#ifndef INIT_TRANSACTIONS_H
#define INIT_TRANSACTIONS_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

struct TypedException;

namespace transactions {
class TransactionsDBStorage;
class TransactionsJavascript;
class Transactions;
}

namespace auth {
class Auth;
class AuthJavascript;
}

namespace wallets {
class Wallets;
class WalletsJavascript;
}

class MainWindow;
class NsLookup;

namespace initializer {

class InitializerJavascript;

class InitTransactions: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<transactions::TransactionsJavascript*, transactions::Transactions*>;

    using Callback = std::function<void()>;

public:

    InitTransactions(QThread *mainThread, Initializer &manager);

    ~InitTransactions() override;

    void completeImpl() override;

    Return initialize(std::shared_future<MainWindow*> mainWindow, std::shared_future<NsLookup*> nsLookup, std::shared_future<std::pair<auth::Auth*, auth::AuthJavascript*>> auth, std::shared_future<std::pair<wallets::Wallets*, wallets::WalletsJavascript*>> wallets);

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

    void callbackCall(const InitTransactions::Callback &callback);

private slots:

    void onCallbackCall(const InitTransactions::Callback &callback);

private:

    std::unique_ptr<transactions::TransactionsDBStorage> database;
    std::unique_ptr<transactions::TransactionsJavascript> txJavascript;
    std::unique_ptr<transactions::Transactions> txManager;

};

}

#endif // INIT_TRANSACTIONS_H
