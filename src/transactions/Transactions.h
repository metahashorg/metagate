#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

#include <QObject>
#include <QString>

#include <functional>
#include <vector>
#include <map>

#include "client.h"
#include "TimerClass.h"

class NsLookup;
struct TypedException;

namespace transactions {

class TransactionsJavascript;
class TransactionsDBStorage;
struct BalanceResponse;
struct Transaction;

class Transactions : public TimerClass {
    Q_OBJECT
public:

    using RegisterAddressCallback = std::function<void(const TypedException &exception)>;

    using GetTxsCallback = std::function<void(const std::vector<Transaction> &txs, const TypedException &exception)>;

public:

    explicit Transactions(NsLookup &nsLookup, TransactionsJavascript &javascriptWrapper, TransactionsDBStorage &db, QObject *parent = nullptr);

signals:

    void registerAddress(const QString &currency, const QString &address, const QString &type, const RegisterAddressCallback &callback);

    void getTxs(QString address, QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void getTxsAll(QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

public slots:

    void onRegisterAddress(const QString &currency, const QString &address, const QString &type, const RegisterAddressCallback &callback);

    void onGetTxs(QString address, QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void onGetTxsAll(QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

private slots:

    void onCallbackCall(std::function<void()> callback);

    void onRun();

    void onTimerEvent();

private:

    void processAddressMth(const QString &address, const QString &currency, const std::vector<QString> &servers);

    void newBalance(const QString &address, const QString &currency, const BalanceResponse &balance, const std::vector<Transaction> &txs);

    template<typename Func>
    void runCallback(const Func &callback);

private:

    NsLookup &nsLookup;

    TransactionsJavascript &javascriptWrapper;

    TransactionsDBStorage &db;

    SimpleClient client;

    std::map<std::pair<QString, QString>, bool> getFullTxs;
};

}

#endif // TRANSACTIONS_H
