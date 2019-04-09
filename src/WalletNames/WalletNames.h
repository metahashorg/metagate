#ifndef WALLETNAMES_H
#define WALLETNAMES_H

#include <functional>

#include "TimerClass.h"
#include "CallbackWrapper.h"

#include "WalletInfo.h"

class JavascriptWrapper;

namespace wallet_names {

class WalletNamesDbStorage;

class WalletNames: public TimerClass {
    Q_OBJECT   
public:

    using Callback = std::function<void()>;

    using AddWalletsNamesCallback = CallbackWrapper<void()>;

    using SaveWalletNameCallback = CallbackWrapper<void()>;

    using GetWalletNameCallback = CallbackWrapper<void(const QString &name)>;

    using GetAllWalletsCurrencyCallback = CallbackWrapper<void(const std::vector<WalletInfo> &thisWallets, const std::vector<WalletInfo> &otherWallets)>;

public:

    WalletNames(WalletNamesDbStorage &db, JavascriptWrapper &javascriptWrapper);

signals:

    void addOrUpdateWallets(const std::vector<WalletInfo> &infos, const AddWalletsNamesCallback &callback);

    void saveWalletName(const QString &address, const QString &name, const SaveWalletNameCallback &callback);

    void getWalletName(const QString &address, const GetWalletNameCallback &callback);

    void getAllWalletsCurrency(const QString &currency, const GetAllWalletsCurrencyCallback &callback);

private slots:

    void onAddOrUpdateWallets(const std::vector<WalletInfo> &infos, const AddWalletsNamesCallback &callback);

    void onSaveWalletName(const QString &address, const QString &name, const SaveWalletNameCallback &callback);

    void onGetWalletName(const QString &address, const GetWalletNameCallback &callback);

    void onGetAllWalletsCurrency(const QString &currency, const GetAllWalletsCurrencyCallback &callback);

signals:

    void updatedWalletName(const QString &address, const QString &name);

    void walletsFlushed();

signals:

    void callbackCall(WalletNames::Callback callback);

private slots:

    void onCallbackCall(WalletNames::Callback callback);

    void onRun();

    void onTimerEvent();

private:

    WalletNamesDbStorage &db;

    JavascriptWrapper &javascriptWrapper;

    std::function<void(const std::function<void()> &callback)> signalFunc;

};

} // namespace wallet_names

#endif // WALLETNAMES_H
