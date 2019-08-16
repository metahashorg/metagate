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

class WebSocketClient;
class MainWindow;

namespace metagate {

class MetaGate: public ManagerWrapper {
    Q_OBJECT
public:

    MetaGate(MainWindow &mainWindow, auth::Auth &authManager, wallets::Wallets &wallets, WebSocketClient &wssClient, const QString &applicationVersion);

private:

    QByteArray getUtmData();

    void sendAppInfoToWss(const QString &userName, const std::vector<wallets::WalletInfo> &keysTmh, const std::vector<wallets::WalletInfo> &keysMhc);

    void sendAppInfoToWss1();

    void sendAppInfoToWss2(const QString &userName);

private slots:

    void onMhcWalletChanged(bool isMhc, const QString &address, const QString &userName);

    void onMhcWatchWalletsChanged(bool isMhc, const std::vector<std::pair<QString, QString>> &created, const QString &username);

    void onLogined(bool isInit, const QString &login, const QString &token);

signals:

    void sendCommandLineMessageToWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText);

private slots:

    void onSendCommandLineMessageToWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText);

private:

    MainWindow &mainWindow;

    wallets::Wallets &wallets;

    WebSocketClient &wssClient;

    QString applicationVersion;

    QString hardwareId;

    QString utmData;

    QString sendedUserName;

    QString currentUserName;

};

} // namespace metagate

#endif // METAGATE_H
