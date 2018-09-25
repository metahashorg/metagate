#ifndef TRANSACTIONSJAVASCRIPT_H
#define TRANSACTIONSJAVASCRIPT_H

#include <QObject>

#include <functional>

struct TypedException;

namespace transactions {

class Transactions;

struct BalanceInfo;
struct Transaction;

class TransactionsJavascript
    : public QObject
{    
    Q_OBJECT
public:

    using Callback = std::function<void()>;

public:

    explicit TransactionsJavascript(QObject *parent = nullptr);

    void setTransactions(Transactions &trans) {
        transactionsManager = &trans;
    }

signals:

    void jsRunSig(QString jsString);

    void callbackCall(const Callback &callback);

signals:

    void newBalanceSig(const QString &address, const QString &currency, const BalanceInfo &balance);

    void sendedTransactionsResponseSig(const QString &requestId, const QString &server, const QString &response, const TypedException &error);

    void transactionInTorrentSig(const QString &server, const QString &txHash, const Transaction &tx, const TypedException &error);

    void transactionStatusChangedSig(const QString &address, const QString &currency, const QString &txHash, const Transaction &tx);

public slots:

    void onCallbackCall(const Callback &callback);

private slots:

    void onNewBalance(const QString &address, const QString &currency, const BalanceInfo &balance);

    void onSendedTransactionsResponse(const QString &requestId, const QString &server, const QString &response, const TypedException &error);

    void onTransactionInTorrent(const QString &server, const QString &txHash, const Transaction &tx, const TypedException &error);

    void onTransactionStatusChanged(const QString &address, const QString &currency, const QString &txHash, const Transaction &tx);

public slots:

    Q_INVOKABLE void registerAddress(QString address, QString currency, QString type, QString group, QString name);

    Q_INVOKABLE void registerAddresses(QString addressesJson);

    Q_INVOKABLE void getAddresses(QString group);

    Q_INVOKABLE void setCurrentGroup(QString group);

    Q_INVOKABLE void getTxs(QString address, QString currency, QString fromTx, int count, bool asc);

    Q_INVOKABLE void getTxsAll(QString currency, QString fromTx, int count, bool asc);

    Q_INVOKABLE void getTxs2(QString address, QString currency, int from, int count, bool asc);

    Q_INVOKABLE void getTxsAll2(QString currency, int from, int count, bool asc);

    Q_INVOKABLE void calcBalance(QString address, QString currency);

    Q_INVOKABLE void getTxFromServer(QString txHash, QString type);

    Q_INVOKABLE void getLastUpdatedBalance(QString currency);

private:

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:

    Transactions *transactionsManager;
};

}

#endif // TRANSACTIONSJAVASCRIPT_H
