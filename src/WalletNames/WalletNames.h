#ifndef WALLETNAMES_H
#define WALLETNAMES_H

#include <functional>

#include "qt_utilites/TimerClass.h"
#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include "utilites/RequestId.h"

#include "WalletInfo.h"

#include "Network/WebSocketClient.h"
#include "Network/SimpleClient.h"

class JavascriptWrapper;

namespace auth {
class Auth;
}

namespace wallet_names {

class WalletNamesDbStorage;

class WalletNames: public ManagerWrapper, public TimerClass {
    Q_OBJECT
public:

    using AddWalletsNamesCallback = CallbackWrapper<void()>;

    using SaveWalletNameCallback = CallbackWrapper<void()>;

    using GetWalletNameCallback = CallbackWrapper<void(const QString &name)>;

    using GetAllWalletsCurrencyCallback = CallbackWrapper<void(const std::vector<WalletInfo> &thisWallets, const std::vector<WalletInfo> &otherWallets)>;

public:

    WalletNames(WalletNamesDbStorage &db, JavascriptWrapper &javascriptWrapper, auth::Auth &authManager, WebSocketClient &client);

    ~WalletNames() override;

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

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

private slots:

    void onWssMessageReceived(QString message);

    void onLogined(bool isInit, const QString login);

private:

    void processWalletsList(const std::vector<WalletInfo> &wallets);

    void getAllWallets();

    void sendAllWallets();

    void getAllWalletsApps();

private:

    enum class StateRequest {
        NotRequest, Requested, Intercepted
    };

private:

    WalletNamesDbStorage &db;

    JavascriptWrapper &javascriptWrapper;

    auth::Auth &authManager;

    WebSocketClient &client;

    QString token;

    QString hwid;

    RequestId id;

    StateRequest stateRequest = StateRequest::NotRequest;

    QString serverName;

    seconds timeout;

    SimpleClient httpClient;

};

} // namespace wallet_names

#endif // WALLETNAMES_H
