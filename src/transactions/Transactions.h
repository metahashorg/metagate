#ifndef TRANSACTIONS_H
#define TRANSACTIONS_H

#include <QObject>
#include <QString>

#include <functional>
#include <vector>
#include <map>
#include <set>

#include "client.h"
#include "HttpClient.h"
#include "TimerClass.h"

#include "CallbackWrapper.h"

#include "Transaction.h"

class NsLookup;
struct TypedException;

class JavascriptWrapper;

namespace transactions {

class TransactionsJavascript;
class TransactionsDBStorage;
enum class DelegateStatus;

class Transactions : public TimerClass {
    Q_OBJECT
private:

    using TransactionHash = std::string;

    class SendedTransactionWatcher {
    public:

        SendedTransactionWatcher(const SendedTransactionWatcher &) = delete;
        SendedTransactionWatcher(SendedTransactionWatcher &&) = delete;
        SendedTransactionWatcher& operator=(const SendedTransactionWatcher &) = delete;
        SendedTransactionWatcher& operator=(SendedTransactionWatcher &&) = delete;

        SendedTransactionWatcher(Transactions &txManager, const QString &requestId, const TransactionHash &hash, const time_point &startTime, const std::vector<QString> &servers, const seconds &timeout)
            : startTime(startTime)
            , timeout(timeout)
            , txManager(txManager)
            , requestId(requestId)
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

        bool isTimeout(const time_point &now) const {
            return now - startTime >= timeout;
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

    public:

        const QString requestId;

    private:
        const time_point startTime;
        const seconds timeout;

        Transactions &txManager;

        TransactionHash hash;

        std::set<QString> servers;
        std::set<QString> allServers;
        std::map<QString, QString> errors;
    };

    struct ServersStruct {
        int countRequests = 0;
        QString currency;

        ServersStruct(const QString &currency)
            : currency(currency)
        {}
    };

public:

    using SignalFunc = std::function<void(const std::function<void()> &callback)>;

    using RegisterAddressCallback = CallbackWrapper<void()>;

    using GetTxsCallback = CallbackWrapper<void(const std::vector<Transaction> &txs)>;

    using CalcBalanceCallback = CallbackWrapper<void(const BalanceInfo &txs)>;

    using SetCurrentGroupCallback = CallbackWrapper<void()>;

    using GetAddressesCallback = CallbackWrapper<void(const std::vector<AddressInfo> &result)>;

    using GetTxCallback = CallbackWrapper<void(const Transaction &txs)>;

    using GetLastUpdateCallback = CallbackWrapper<void(const system_time_point &lastUpdate, const system_time_point &now)>;

    using GetNonceCallback = CallbackWrapper<void(size_t nonce, const QString &serverError)>;

    using SendTransactionCallback = CallbackWrapper<void()>;

    using ClearDbCallback = CallbackWrapper<void()>;

    using Callback = std::function<void()>;

public:

    explicit Transactions(NsLookup &nsLookup, TransactionsJavascript &javascriptWrapper, TransactionsDBStorage &db, QObject *parent = nullptr);

    ~Transactions();

signals:

    void callbackCall(Transactions::Callback callback);

signals:

    void registerAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback);

    void getAddresses(const QString &group, const GetAddressesCallback &callback);

    void setCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback);

    void getTxs(const QString &address, const QString &currency, const QString &fromTx, int count, bool asc, const GetTxsCallback &callback);

    void getTxs2(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void getTxsAll(const QString &currency, const QString &fromTx, int count, bool asc, const GetTxsCallback &callback);

    void getTxsAll2(const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void getForgingTxs(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void getLastForgingTx(const QString &address, const QString &currency, const GetTxCallback &callback);

    void calcBalance(const QString &address, const QString &currency, const CalcBalanceCallback &callback);

    void getNonce(const QString &requestId, const QString &from, const SendParameters &sendParams, const GetNonceCallback &callback);

    void sendTransaction(const QString &requestId, const QString &to, const QString &value, size_t nonce, const QString &data, const QString &fee, const QString &pubkey, const QString &sign, const SendParameters &sendParams, const SendTransactionCallback &callback);

    void getTxFromServer(const QString &txHash, const QString &type, const GetTxCallback &callback);

    void getLastUpdateBalance(const QString &currency, const GetLastUpdateCallback &callback);

    void clearDb(const QString &currency, const ClearDbCallback &callback);

public slots:

    void onRegisterAddresses(const std::vector<AddressInfo> &addresses, const RegisterAddressCallback &callback);

    void onGetAddresses(const QString &group, const GetAddressesCallback &callback);

    void onSetCurrentGroup(const QString &group, const SetCurrentGroupCallback &callback);

    void onGetTxs(const QString &address, const QString &currency, const QString &fromTx, int count, bool asc, const GetTxsCallback &callback);

    void onGetTxs2(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void onGetTxsAll(const QString &currency, const QString &fromTx, int count, bool asc, const GetTxsCallback &callback);

    void onGetTxsAll2(const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void onGetForgingTxs(const QString &address, const QString &currency, int from, int count, bool asc, const GetTxsCallback &callback);

    void onGetLastForgingTx(const QString &address, const QString &currency, const GetTxCallback &callback);

    void onCalcBalance(const QString &address, const QString &currency, const CalcBalanceCallback &callback);

    void onGetNonce(const QString &requestId, const QString &from, const SendParameters &sendParams, const GetNonceCallback &callback);

    void onSendTransaction(const QString &requestId, const QString &to, const QString &value, size_t nonce, const QString &data, const QString &fee, const QString &pubkey, const QString &sign, const SendParameters &sendParams, const SendTransactionCallback &callback);

    void onGetTxFromServer(const QString &txHash, const QString &type, const GetTxCallback &callback);

    void onGetLastUpdateBalance(const QString &currency, const GetLastUpdateCallback &callback);

    void onClearDb(const QString &currency, const ClearDbCallback &callback);

private slots:

    void onCallbackCall(Transactions::Callback callback);

    void onRun();

    void onTimerEvent();

    void onFindTxOnTorrentEvent();

private:

    void processCheckTxs(const QString &address, const QString &currency, const std::vector<QString> &servers);

    void processCheckTxsOneServer(const QString &address, const QString &currency, const QUrl &server);

    void processCheckTxsInternal(const QString &address, const QString &currency, const QUrl &server, const Transaction &tx, int64_t serverBlockNumber);

    void processAddressMth(const QString &address, const QString &currency, const std::vector<QString> &servers, const std::shared_ptr<ServersStruct> &servStruct, const std::vector<QString> &pendingTxs);

    void processPendingsMth(const std::vector<QString> &servers);

    uint64_t calcCountTxs(const QString &address, const QString &currency) const;

    void newBalance(const QString &address, const QString &currency, uint64_t savedCountTxs, const BalanceInfo &balance, const std::vector<Transaction> &txs, const std::shared_ptr<ServersStruct> &servStruct);

    void updateBalanceTime(const QString &currency, const std::shared_ptr<ServersStruct> &servStruct);

    std::vector<AddressInfo> getAddressesInfos(const QString &group);

    BalanceInfo getBalance(const QString &address, const QString &currency);

    void addToSendTxWatcher(const QString &requestId, const TransactionHash &hash, size_t countServers, const std::vector<QString> &servers, const seconds &timeout);

    void sendErrorGetTx(const QString &requestId, const TransactionHash &hash, const QString &server);

    void fetchBalanceAddress(const QString &address);

private:

    NsLookup &nsLookup;

    TransactionsJavascript &javascriptWrapper;

    TransactionsDBStorage &db;

    SimpleClient client;

    HttpSimpleClient tcpClient;

    QString currentGroup;

    QTimer timerSendTx;

    std::map<TransactionHash, SendedTransactionWatcher> sendTxWathcers;

    std::map<QString, system_time_point> lastSuccessUpdateTimestamps;

    std::vector<QString> pendingTxsAfterSend;

    seconds timeout;

    time_point lastCheckTxsTime;

    std::vector<AddressInfo> addressesInfos;

    size_t posInAddressInfos;
};

SendParameters parseSendParams(const QString &paramsJson);

}

#endif // TRANSACTIONS_H
