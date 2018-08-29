#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

#include <QObject>
#include <QString>

#include <functional>
#include <vector>
#include <map>
#include <set>

#include "client.h"
#include "httpclient.h"
#include "TimerClass.h"

class NsLookup;
struct TypedException;

namespace transactions {

class TransactionsJavascript;
class TransactionsDBStorage;
struct BalanceInfo;
struct Transaction;
struct AddressInfo;

class Transactions : public TimerClass {
    Q_OBJECT
private:

    using TransactionHash = std::string;

    class SendedTransactionWatcher {
    public:

        const time_point startTime;

        SendedTransactionWatcher(const SendedTransactionWatcher &) = delete;
        SendedTransactionWatcher(SendedTransactionWatcher &&) = delete;
        SendedTransactionWatcher& operator=(const SendedTransactionWatcher &) = delete;
        SendedTransactionWatcher& operator=(SendedTransactionWatcher &&) = delete;

        SendedTransactionWatcher(Transactions &txManager, const TransactionHash &hash, const time_point &startTime, const std::vector<QString> &servers)
            : startTime(startTime)
            , txManager(txManager)
            , hash(hash)
            , servers(servers.begin(), servers.end())
            , allServers(servers.begin(), servers.end())
        {}

        ~SendedTransactionWatcher();

        std::set<QString> getServersCopy() const {
            return servers;
        }

        bool isEmpty() const {
            return allServers.empty();
        }

        void removeServer(const QString &server) {
            servers.erase(server);
        }

        void returnServer(const QString &server) {
            if (allServers.find(server) != allServers.end()) {
                servers.insert(server);
            }
        }

        void okServer(const QString &server) {
            if (allServers.find(server) != allServers.end()) {
                servers.erase(server);
                allServers.erase(server);
            }
        }

        void setError(const QString &server, const QString &error) {
            errors[server] = error;
        }

    private:
        Transactions &txManager;

        TransactionHash hash;

        std::set<QString> servers;
        std::set<QString> allServers;
        std::map<QString, QString> errors;
    };

public:

    using RegisterAddressCallback = std::function<void(const TypedException &exception)>;

    using GetTxsCallback = std::function<void(const std::vector<Transaction> &txs, const TypedException &exception)>;

    using CalcBalanceCallback = std::function<void(const BalanceInfo &txs, const TypedException &exception)>;

    using SetCurrentGroupCallback = std::function<void(const TypedException &exception)>;

    using GetAddressesCallback = std::function<void(const std::vector<AddressInfo> &result, const TypedException &exception)>;

    using GetTxCallback = std::function<void(const Transaction &txs, const TypedException &exception)>;

    using Callback = std::function<void()>;

public:

    explicit Transactions(NsLookup &nsLookup, TransactionsJavascript &javascriptWrapper, TransactionsDBStorage &db, QObject *parent = nullptr);

signals:

    void registerAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback);

    void getAddresses(const QString &group, const GetAddressesCallback &callback);

    void setCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback);

    void getTxs(QString address, QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void getTxs2(QString address, QString currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void getTxsAll(QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void getTxsAll2(QString currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void calcBalance(QString address, QString currency, const CalcBalanceCallback &callback);

    void sendTransaction(QString requestId, int countServersSend, int countServersGet, QString to, QString value, QString nonce, QString data, QString fee, QString pubkey, QString sign, QString typeSend, QString typeGet);

    void getTxFromServer(const QString &txHash, const QString &type, const GetTxCallback &callback);

public slots:

    void onRegisterAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback);

    void onGetAddresses(const QString &group, const GetAddressesCallback &callback);

    void onSetCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback);

    void onGetTxs(QString address, QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void onGetTxs2(QString address, QString currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void onGetTxsAll(QString currency, QString fromTx, int count, bool asc, const GetTxsCallback &callback);

    void onGetTxsAll2(QString currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void onCalcBalance(QString address, QString currency, const CalcBalanceCallback &callback);

    void onSendTransaction(QString requestId, int countServersSend, int countServersGet, QString to, QString value, QString nonce, QString data, QString fee, QString pubkey, QString sign, QString typeSend, QString typeGet);

    void onGetTxFromServer(const QString &txHash, const QString &type, const GetTxCallback &callback);

private slots:

    void onCallbackCall(Callback callback);

    void onRun();

    void onTimerEvent();

    void onSendTxEvent();

private:

    void processAddressMth(const QString &address, const QString &currency, const std::vector<QString> &servers);

    void newBalance(const QString &address, const QString &currency, const BalanceInfo &balance, const std::vector<Transaction> &txs);

    template<typename Func>
    void runCallback(const Func &callback);

    std::vector<AddressInfo> getAddressesInfos(const QString &group);

    void addToSendTxWatcher(const TransactionHash &hash, size_t countServers, const QString &group);

    void sendErrorGetTx(const TransactionHash &hash, const QString &server);

private:

    NsLookup &nsLookup;

    TransactionsJavascript &javascriptWrapper;

    TransactionsDBStorage &db;

    SimpleClient client;

    HttpSimpleClient tcpClient;

    std::map<std::pair<QString, QString>, bool> getFullTxs;

    QString currentGroup;

    QTimer timerSendTx;

    std::map<TransactionHash, SendedTransactionWatcher> sendTxWathcers;
};

}

#endif // TRANSACTIONS_H
