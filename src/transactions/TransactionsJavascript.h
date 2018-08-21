#ifndef TRANSACTIONSJAVASCRIPT_H
#define TRANSACTIONSJAVASCRIPT_H

#include <QObject>

#include <functional>

#include "TypedException.h"

#include "Transaction.h"

namespace transactions {

class Transactions;

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

    void newBalanceSig(const QString &address, const QString &currency, const BalanceResponse &balance);

public slots:

    void onCallbackCall(const Callback &callback);

private slots:

    void onNewBalance(const QString &address, const QString &currency, const BalanceResponse &balance);

public slots:

    Q_INVOKABLE void registerAddress(QString address, QString currency, QString type);

private:

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:

    Transactions *transactionsManager;
};

}

#endif // TRANSACTIONSJAVASCRIPT_H
