#ifndef WALLETNAMESJAVASCRIPT_H
#define WALLETNAMESJAVASCRIPT_H

#include <functional>

#include <QObject>

#include "TypedException.h"

namespace wallet_names {

class WalletNames;

class WalletNamesJavascript :public QObject {
    Q_OBJECT
public:

    using Callback = std::function<void()>;

public:

    explicit WalletNamesJavascript(WalletNames& walletNames, QObject *parent = nullptr);

signals:

    void jsRunSig(QString jsString);

    void callbackCall(const WalletNamesJavascript::Callback &callback);

public slots:

    Q_INVOKABLE void saveKeyName(QString address, QString name);

    Q_INVOKABLE void getKeyName(QString address);

    Q_INVOKABLE void getAllWalletsInCurrency(QString currency);

private slots:

    void onCallbackCall(const WalletNamesJavascript::Callback &callback);

private slots:

    void onUpdatedWalletName(const QString &address, const QString &name);

    void onWalletsFlushed();

private:

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:

    WalletNames &manager;

    std::function<void(const std::function<void()> &callback)> signalFunc;

};

} // namespace wallet_names

#endif // WALLETNAMESJAVASCRIPT_H
