#ifndef METAGATE_H
#define METAGATE_H

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

namespace wallets {
class Wallets;
struct WalletInfo;
}

namespace auth {
class Auth;
}

namespace transactions {
class Transactions;
class BalanceInfo;
}

class WebSocketClient;
class MainWindow;
class NsLookup;
class NetwrokTesting;

struct NodeTypeStatus;
struct DnsErrorDetails;
struct NetworkTestingTestResult;

namespace metagate {

class MetaGate: public ManagerWrapper {
    Q_OBJECT
public:

    using UpdateAndReloadApplicationCallback = CallbackWrapper<void(bool result)>;

    using GetAppInfoCallback = CallbackWrapper<void(const QString &hardwareId, bool isProductionSetup, const QString &versionString, const QString &gitHash)>;

    using MetaOnlineCallback = CallbackWrapper<void(bool result)>;

    using ClearNsLookupCallback = CallbackWrapper<void(bool result)>;

    using SendMessageToWssCallback = CallbackWrapper<void(bool result)>;

    using SetForgingActiveCallback = CallbackWrapper<void(bool result)>;

    using GetForgingActiveCallback = CallbackWrapper<void(bool result)>;

    using GetNetworkStatusCallback = CallbackWrapper<void(const std::vector<NetworkTestingTestResult> &networkTestsResults, const std::vector<NodeTypeStatus> &nodeStatuses, const DnsErrorDetails &dnsError)>;

public:

    MetaGate(MainWindow &mainWindow, auth::Auth &authManager, wallets::Wallets &wallets, transactions::Transactions &transactions, WebSocketClient &wssClient, NsLookup &nsLookup, NetwrokTesting &networkTesting, const QString &applicationVersion);

signals:

    void lineEditReturnPressedSig(QString text);

    void metaOnlineResponse(const QString &response);

    void showExchangePopup(const QString &type);

signals:

    void updateAndReloadApplication(const UpdateAndReloadApplicationCallback &callback);

    void exitApplication();

    void restartBrowser();

    void getAppInfo(const GetAppInfoCallback &callback);

    void lineEditReturnPressed(const QString &text);

    void metaOnline(const MetaOnlineCallback &callback);

    void clearNsLookup(const ClearNsLookupCallback &callback);

    void sendMessageToWss(const QString &message, const SendMessageToWssCallback &callback);

    void setForgingActive(bool isActive, const SetForgingActiveCallback &callback);

    void getForgingIsActive(const GetForgingActiveCallback &callback);

    void getNetworkStatus(const GetNetworkStatusCallback &callback);

private slots:

    void onUpdateAndReloadApplication(const UpdateAndReloadApplicationCallback &callback);

    void onExitApplication();

    void onRestartBrowser();

    void onGetAppInfo(const GetAppInfoCallback &callback);

    void onLineEditReturnPressed(const QString &text);

    void onMetaOnline(const MetaOnlineCallback &callback);

    void onClearNsLookup(const ClearNsLookupCallback &callback);

    void onSendMessageToWss(const QString &message, const SendMessageToWssCallback &callback);

    void onSetForgingActive(bool isActive, const SetForgingActiveCallback &callback);

    void onGetForgingIsActive(const GetForgingActiveCallback &callback);

    void onGetNetworkStatus(const GetNetworkStatusCallback &callback);

    void onTestTorrentResult(const QString &id, bool res, const QString &descr, const std::vector<transactions::BalanceInfo> &result);


private:

    QByteArray getUtmData();

    void sendAppInfoToWss(const QString &userName, const std::vector<wallets::WalletInfo> &keysTmh, const std::vector<wallets::WalletInfo> &keysMhc);

    void sendAppInfoToWss1();

    void sendAppInfoToWss2(const QString &userName);

private slots:

    void onMhcWalletChanged(bool isMhc, const QString &address, const QString &userName);

    void onMhcWatchWalletsChanged(bool isMhc, const std::vector<std::pair<QString, QString>> &created, const QString &username);

    void onLogined(bool isInit, const QString &login, const QString &token);

    void onWssMessageReceived(const QString &message);

signals:

    void sendCommandLineMessageToWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText);

private slots:

    void onSendCommandLineMessageToWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText);

private:

    MainWindow &mainWindow;

    wallets::Wallets &wallets;

    transactions::Transactions &transactions;

    WebSocketClient &wssClient;

    NsLookup &nsLookup;

    NetwrokTesting &networkTesting;

    QString applicationVersion;

    QString hardwareId;

    QString utmData;

    QString sendedUserName;

    QString currentUserName;

};

} // namespace metagate

#endif // METAGATE_H
