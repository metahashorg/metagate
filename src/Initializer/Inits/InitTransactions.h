#ifndef INIT_TRANSACTIONS_H
#define INIT_TRANSACTIONS_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

#include "../SharedFuture.h"

struct TypedException;

namespace transactions {
class TransactionsDBStorage;
class TransactionsJavascript;
class Transactions;
}

namespace auth {
class Auth;
}

namespace wallets {
class Wallets;
}

class MainWindow;
class NsLookup;
class InfrastructureNsLookup;

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

    Return initialize(SharedFuture<MainWindow> mainWindow, std::shared_future<std::pair<NsLookup*, InfrastructureNsLookup*>> nsLookup, SharedFuture<auth::Auth> auth, SharedFuture<wallets::Wallets> wallets);

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
