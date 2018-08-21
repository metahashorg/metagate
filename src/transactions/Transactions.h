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

    using CalcBalanceCallback = std::function<void(const BalanceResponse &txs, const TypedException &exception)>;

    using SetCurrentGroupCallback = std::function<void(const TypedException &exception)>;

public:

    struct AddressInfo {
        QString currency;
        QString address;
        QString type;
        QString group;
    };

public:

    explicit Transactions(NsLookup &nsLookup, TransactionsJavascript &javascriptWrapper, TransactionsDBStorage &db, QObject *parent = nullptr);

signals:

    void registerAddress(const QString &currency, const QString &address, const QString &type, const QString &group, const RegisterAddressCallback &callback);

    void registerAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback);

    void setCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback);

    void getTxs(QString address, QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void getTxsAll(QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void calcBalance(QString address, QString currency, const CalcBalanceCallback &callback);

public slots:

    void onRegisterAddress(const QString &currency, const QString &address, const QString &type, const QString &group, const RegisterAddressCallback &callback);

    void onRegisterAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback);

    void onSetCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback);

    void onGetTxs(QString address, QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void onGetTxsAll(QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void onCalcBalance(QString address, QString currency, const CalcBalanceCallback &callback);

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

    QString currentGroup;
};

}

#endif // TRANSACTIONS_H
