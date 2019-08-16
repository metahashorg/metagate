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

#include "Wallets/Wallets.h"
#include "auth/Auth.h"

#include "Network/WebSocketClient.h"

#include "mainwindow.h"

#include <QDir>
#include <QApplication>
#include <QSettings>

SET_LOG_NAMESPACE("MG");

namespace metagate {

MetaGate::MetaGate(MainWindow &mainWindow, auth::Auth &authManager, wallets::Wallets &wallets, WebSocketClient &wssClient, const QString &applicationVersion)
    : mainWindow(mainWindow)
    , wallets(wallets)
    , wssClient(wssClient)
    , applicationVersion(applicationVersion)
{
    hardwareId = QString::fromStdString(::getMachineUid());
    utmData = QString::fromLatin1(getUtmData());

    Q_CONNECT(&wallets, &wallets::Wallets::mhcWalletCreated, this, &MetaGate::onMhcWalletChanged);
    Q_CONNECT(&wallets, &wallets::Wallets::mhcWatchWalletCreated, this, &MetaGate::onMhcWalletChanged);
    Q_CONNECT(&wallets, &wallets::Wallets::mhcWatchWalletRemoved, this, &MetaGate::onMhcWalletChanged);
    Q_CONNECT(&wallets, &wallets::Wallets::watchWalletsAdded, this, &MetaGate::onMhcWatchWalletsChanged);

    Q_CONNECT(&authManager, &auth::Auth::logined2, this, &MetaGate::onLogined);

    Q_CONNECT(this, &MetaGate::sendCommandLineMessageToWss, this, &MetaGate::onSendCommandLineMessageToWss);

    sendAppInfoToWss1();

    emit authManager.reEmit();
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

void MetaGate::onLogined(bool /*isInit*/, const QString &login, const QString &token_) {
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

} // namespace metagate
