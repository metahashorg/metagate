#include "MetaGate.h"

#include "MetaGateMessages.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "Log.h"
#include "check.h"
#include "utilites/machine_uid.h"
#include "Paths.h"
#include "utilites/platform.h"
#include "StopApplication.h"

#include "Wallets/Wallets.h"
#include "auth/Auth.h"
#include "transactions/Transactions.h"

#include "Network/WebSocketClient.h"
#include "Network/NetwrokTesting.h"
#include "NsLookup/NsLookup.h"

#include "mainwindow.h"

#include <QDir>
#include <QApplication>
#include <QSettings>
#include <QJsonDocument>

SET_LOG_NAMESPACE("MG");

namespace metagate {

MetaGate::MetaGate(MainWindow &mainWindow, auth::Auth &authManager, wallets::Wallets &wallets, transactions::Transactions &transactions, WebSocketClient &wssClient, NsLookup &nsLookup, NetwrokTesting &networkTesting, const QString &applicationVersion)
    : mainWindow(mainWindow)
    , wallets(wallets)
    , transactions(transactions)
    , wssClient(wssClient)
    , nsLookup(nsLookup)
    , networkTesting(networkTesting)
    , applicationVersion(applicationVersion)
{
    hardwareId = QString::fromStdString(::getMachineUid());
    utmData = QString::fromLatin1(getUtmData());

    Q_CONNECT(&wallets, &wallets::Wallets::mhcWalletCreated, this, &MetaGate::onMhcWalletChanged);
    Q_CONNECT(&wallets, &wallets::Wallets::mhcWatchWalletCreated, this, &MetaGate::onMhcWalletChanged);
    Q_CONNECT(&wallets, &wallets::Wallets::mhcWatchWalletRemoved, this, &MetaGate::onMhcWalletChanged);
    Q_CONNECT(&wallets, &wallets::Wallets::watchWalletsAdded, this, &MetaGate::onMhcWatchWalletsChanged);

    Q_CONNECT(&authManager, &auth::Auth::logined2, this, &MetaGate::onLogined);

    Q_CONNECT(&transactions, &transactions::Transactions::getBalancesFromTorrentResult, this, &MetaGate::onTestTorrentResult);

    Q_CONNECT(this, &MetaGate::sendCommandLineMessageToWss, this, &MetaGate::onSendCommandLineMessageToWss);

    Q_CONNECT(&wssClient, &WebSocketClient::messageReceived, this, &MetaGate::onWssMessageReceived);

    Q_CONNECT(this, &MetaGate::updateAndReloadApplication, this, &MetaGate::onUpdateAndReloadApplication);
    Q_CONNECT(this, &MetaGate::exitApplication, this, &MetaGate::onExitApplication);
    Q_CONNECT(this, &MetaGate::restartBrowser, this, &MetaGate::onRestartBrowser);
    Q_CONNECT(this, &MetaGate::getAppInfo, this, &MetaGate::onGetAppInfo);
    Q_CONNECT(this, &MetaGate::metaOnline, this, &MetaGate::onMetaOnline);
    Q_CONNECT(this, &MetaGate::clearNsLookup, this, &MetaGate::onClearNsLookup);
    Q_CONNECT(this, &MetaGate::lineEditReturnPressed, this, &MetaGate::onLineEditReturnPressed);
    Q_CONNECT(this, &MetaGate::sendMessageToWss, this, &MetaGate::onSendMessageToWss);
    Q_CONNECT(this, &MetaGate::setForgingActive, this, &MetaGate::onSetForgingActive);
    Q_CONNECT(this, &MetaGate::getForgingIsActive, this, &MetaGate::onGetForgingIsActive);
    Q_CONNECT(this, &MetaGate::getNetworkStatus, this, &MetaGate::onGetNetworkStatus);
    Q_CONNECT(this, &MetaGate::activeForgingreEmit, this, &MetaGate::onActiveForgingreEmit);

    Q_REG(UpdateAndReloadApplicationCallback, "UpdateAndReloadApplicationCallback");
    Q_REG(GetAppInfoCallback, "GetAppInfoCallback");
    Q_REG(MetaOnlineCallback, "MetaOnlineCallback");
    Q_REG(ClearNsLookupCallback, "ClearNsLookupCallback");
    Q_REG(SendMessageToWssCallback, "SendMessageToWssCallback");
    Q_REG(SetForgingActiveCallback, "SetForgingActiveCallback");
    Q_REG(GetForgingActiveCallback, "GetForgingActiveCallback");
    Q_REG(GetNetworkStatusCallback, "GetNetworkStatusCallback");

    sendAppInfoToWss1();

    emit authManager.reEmit();
}

void MetaGate::onUpdateAndReloadApplication(const UpdateAndReloadApplicationCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        updateAndRestart();
        return true;
    }, callback);
END_SLOT_WRAPPER
}

void MetaGate::onExitApplication() {
BEGIN_SLOT_WRAPPER
    QApplication::exit(SIMPLE_EXIT);
END_SLOT_WRAPPER
}

void MetaGate::onRestartBrowser() {
BEGIN_SLOT_WRAPPER
    QApplication::exit(RESTART_BROWSER);
END_SLOT_WRAPPER
}

void MetaGate::onGetAppInfo(const GetAppInfoCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        const std::string versionString = VERSION_STRING;
        const std::string gitCommit = GIT_CURRENT_SHA1;

        return std::make_tuple(QString::fromStdString(getMachineUid()), isProductionSetup, QString::fromStdString(versionString), QString::fromStdString(gitCommit));
    }, callback);
END_SLOT_WRAPPER
}

void MetaGate::onLineEditReturnPressed(const QString &text) {
BEGIN_SLOT_WRAPPER
    emit lineEditReturnPressedSig(text);
END_SLOT_WRAPPER
}

void MetaGate::onMetaOnline(const MetaOnlineCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        emit wssClient.sendMessage(metaOnlineMessage());
        return true;
    }, callback);
END_SLOT_WRAPPER
}

void MetaGate::onClearNsLookup(const ClearNsLookupCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this]{
        nsLookup.resetFile();
        return true;
    }, callback);
END_SLOT_WRAPPER
}

void MetaGate::onSendMessageToWss(const QString &message, const SendMessageToWssCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this]{
        emit wssClient.sendMessage(message);
        return true;
    }, callback);
END_SLOT_WRAPPER
}

void MetaGate::onSetForgingActive(bool isActive, const SetForgingActiveCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this]{
        QSettings settings(getRuntimeSettingsPath(), QSettings::IniFormat);
        settings.setValue("forging/enabled", isActive);
        settings.sync();

        sendAppInfoToWss1();

        emit forgingActiveChanged(isActive);

        return true;
    }, callback);
END_SLOT_WRAPPER
}

void MetaGate::onGetForgingIsActive(const GetForgingActiveCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        QSettings settings(getRuntimeSettingsPath(), QSettings::IniFormat);
        const bool isForgingActive = settings.value("forging/enabled", true).toBool();

        return isForgingActive;
    }, callback);
END_SLOT_WRAPPER
}

void MetaGate::onGetNetworkStatus(const GetNetworkStatusCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        networkTesting.getTestResults(NetwrokTesting::GetTestResultsCallback([this, callback](const std::vector<NetworkTestingTestResult> &networkTestsResults) {
            nsLookup.getStatus(NsLookup::GetStatusCallback([networkTestsResults, callback](const std::vector<NodeTypeStatus> &nodeStatuses, const DnsErrorDetails &dnsError) {
                callback.emitCallback(networkTestsResults, nodeStatuses, dnsError);
            }, callback, signalFunc));
        }, callback, signalFunc));
    }, callback);
END_SLOT_WRAPPER
}

void MetaGate::onTestTorrentResult(const QString &id, bool res, const QString &descr, const std::vector<transactions::Transactions::IdBalancePair> &result)
{
BEGIN_SLOT_WRAPPER
    const QString message = makeTestTorrentResponse(id, res, descr, result);
    LOG << "Test torrent result: " << message;
    emit wssClient.sendMessage(message);
    END_SLOT_WRAPPER
}

void MetaGate::onActiveForgingreEmit()
{
    QSettings settings(getRuntimeSettingsPath(), QSettings::IniFormat);
    const bool isForgingActive = settings.value(QStringLiteral("forging/enabled"), false).toBool();
    emit forgingActiveChanged(isForgingActive);
}

QByteArray MetaGate::getUtmData() {
    QDir dir(qApp->applicationDirPath());

    QFile file(dir.filePath(QStringLiteral("installer.ins")));
    if (!file.open(QIODevice::ReadOnly)) {
        return QByteArray();
    }
    QByteArray data = file.read(1024);
    file.close();
    return data;
}

void MetaGate::sendAppInfoToWss(const QString &userName, const std::vector<wallets::WalletInfo> &walletAddressesTmh, const std::vector<wallets::WalletInfo> &walletAddressesMhc) {
    std::vector<QString> keysMth;
    std::vector<QString> keysTmh;
    std::transform(walletAddressesTmh.begin(), walletAddressesTmh.end(), std::back_inserter(keysTmh), [](const wallets::WalletInfo &element) {return element.address;});
    std::transform(walletAddressesMhc.begin(), walletAddressesMhc.end(), std::back_inserter(keysMth), [](const wallets::WalletInfo &element) {return element.address;});

    QSettings settings(getRuntimeSettingsPath(), QSettings::IniFormat);
    const bool isForgingActive = settings.value("forging/enabled", true).toBool();
    const QString message = makeMessageApplicationForWss(hardwareId, utmData, userName, applicationVersion, mainWindow.getCurrentHtmls().lastVersion, isForgingActive, keysTmh, keysMth, isVirtualMachine(), osName);
    LOG << "Send MetaGate info to wss. Count keys " << keysTmh.size() << " " << keysMth.size() << ". " << userName;
    emit wssClient.sendMessage(message);
    emit wssClient.setHelloString(message, "jsWrapper");

    sendedUserName = userName;
}

void MetaGate::sendAppInfoToWss1() {
    emit wallets.getListWallets(wallets::WalletCurrency::Tmh, wallets::Wallets::WalletsListCallback([this](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddressesTmh) {
        emit wallets.getListWallets(wallets::WalletCurrency::Mth, wallets::Wallets::WalletsListCallback([this, savedUserName=userName, walletAddressesTmh](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddressesMhc) {
            CHECK(userName == savedUserName, "Username changed while get wallets");

            sendAppInfoToWss(userName, walletAddressesTmh, walletAddressesMhc);
        }, [](const TypedException &e) {
            LOG << "Error while get keys " << e.description;
        }, signalFunc));
    }, [](const TypedException &e) {
        LOG << "Error while get keys " << e.description;
    }, signalFunc));
}

void MetaGate::sendAppInfoToWss2(const QString &userName) {
    if (sendedUserName == userName) {
        return;
    }
    emit wallets.getListWallets2(wallets::WalletCurrency::Tmh, userName, wallets::Wallets::WalletsListCallback([this](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddressesTmh) {
        emit wallets.getListWallets(wallets::WalletCurrency::Mth, wallets::Wallets::WalletsListCallback([this, savedUserName=userName, walletAddressesTmh](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddressesMhc) {
            CHECK(userName == savedUserName, "Username changed while get wallets");

            sendAppInfoToWss(userName, walletAddressesTmh, walletAddressesMhc);
        }, [](const TypedException &e) {
            LOG << "Error while get keys " << e.description;
        }, signalFunc));
    }, [](const TypedException &e) {
        LOG << "Error while get keys " << e.description;
    }, signalFunc));
}

void MetaGate::onMhcWalletChanged(bool /*isMhc*/, const QString &/*address*/, const QString &userName) {
BEGIN_SLOT_WRAPPER
    if (currentUserName != userName) {
        return;
    }
    sendAppInfoToWss1();
END_SLOT_WRAPPER
}

void MetaGate::onMhcWatchWalletsChanged(bool /*isMhc*/, const std::vector<std::pair<QString, QString>> &/*created*/, const QString &userName) {
BEGIN_SLOT_WRAPPER
    if (currentUserName != userName) {
        return;
    }
    sendAppInfoToWss1();
END_SLOT_WRAPPER
}

void MetaGate::onLogined(bool /*isInit*/, const QString &login, const QString &/*token_*/) {
BEGIN_SLOT_WRAPPER
    sendAppInfoToWss2(login);
    currentUserName = login;
END_SLOT_WRAPPER
}

void MetaGate::onSendCommandLineMessageToWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText) {
BEGIN_SLOT_WRAPPER
    LOG << "Send command line " << line;
    emit wssClient.sendMessage(makeCommandLineMessageForWss(hardwareId, userId, focusCount, line, isEnter, isUserText));
END_SLOT_WRAPPER
}

void MetaGate::onWssMessageReceived(const QString &message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
    CHECK(document.isObject(), "Message not is object");

    const QString appType = parseAppType(document);

    if (appType == QLatin1String("TestTorrent")) {
        QUrl url;
        std::vector<std::pair<QString, QString>> addresses;
        const QString id = parseTestTorrentRequest(document, url, addresses);
        emit transactions.getBalancesFromTorrent(id, url, addresses);
        return;
    } else if (appType == QLatin1String("MetaOnline")) {
        const QString metaOnlineResponse = parseMetaOnlineResponse(document);
        if (!metaOnlineResponse.isEmpty()) {
            emit this->metaOnlineResponse(metaOnlineResponse);
            return;
        }
    } else if (appType == QLatin1String("InEvent")) {
        const std::pair<QString, QString> showExchangeResponse = parseShowExchangePopupResponse(document);
        if (!showExchangeResponse.first.isEmpty() && !showExchangeResponse.second.isEmpty()) {
            if (showExchangeResponse.first == currentUserName) {
                emit showExchangePopup(showExchangeResponse.second);
            }
            return;
        }
    }
END_SLOT_WRAPPER
}

} // namespace metagate
