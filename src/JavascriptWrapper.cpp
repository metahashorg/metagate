#include "JavascriptWrapper.h"

#include <map>
#include <functional>
using namespace std::placeholders;

#include <QApplication>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QSettings>

#include "Wallets/Wallet.h"
#include "Wallets/WalletRsa.h"
#include "Wallets/EthWallet.h"
#include "Wallets/BtcWallet.h"

#include "mainwindow.h"

#include "NsLookup/NsLookup.h"
#include "Network/WebSocketClient.h"

#include "utilites/unzip.h"
#include "check.h"
#include "StopApplication.h"
#include "duration.h"
#include "Log.h"
#include "utilites/utils.h"
#include "TypedException.h"
#include "qt_utilites/SlotWrapper.h"
#include "utilites/platform.h"
#include "Paths.h"
#include "qt_utilites/makeJsFunc.h"
#include "utilites/qrcoder.h"
#include "qt_utilites/QRegister.h"

#include "Module.h"

#include "utilites/machine_uid.h"

#include "Wallets/WalletInfo.h"
#include "Wallets/Wallets.h"

#include "transactions/Transactions.h"
#include "auth/Auth.h"
#include "Utils/UtilsManager.h"
#include "Network/NetwrokTesting.h"

SET_LOG_NAMESPACE("JSW");

const QString JavascriptWrapper::defaultUsername = "_unregistered";

static QString makeCommandLineMessageForWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText) {
    QJsonObject allJson;
    allJson.insert("app", "MetaSearch");
    QJsonObject data;
    data.insert("machine_uid", hardwareId);
    data.insert("user_id", userId);
    data.insert("focus_release_count", static_cast<int>(focusCount));
    data.insert("text", QString(line.toUtf8().toHex()));
    data.insert("is_enter_pressed", isEnter);
    data.insert("is_user_text", isUserText);
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
}

static QString makeMessageApplicationForWss(const QString &hardwareId, const QString &utmData, const QString &userId, const QString &applicationVersion, const QString &interfaceVersion, bool isForgingActive, const std::vector<QString> &keysTmh, const std::vector<QString> &keysMth) {
    QJsonObject allJson;
    allJson.insert("app", "MetaGate");
    QJsonObject data;
    data.insert("machine_uid", hardwareId);
    data.insert("utm_data", utmData);
    data.insert("user_id", userId);
    data.insert("application_ver", applicationVersion);
    data.insert("interface_ver", interfaceVersion);
    data.insert("is_virtual", isVirtualMachine());
    data.insert("isForgingActive", isForgingActive);
    data.insert("os_name", osName);

    QJsonArray keysTmhJson;
    for (const QString &key: keysTmh) {
        keysTmhJson.push_back(key);
    }
    data.insert("keys_tmh", keysTmhJson);

    QJsonArray keysMthJson;
    for (const QString &key: keysMth) {
        keysMthJson.push_back(key);
    }
    data.insert("keys_mth", keysMthJson);

    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
}

static QString makeJsonWallets(const std::vector<std::pair<QString, QString>> &wallets) {
    QJsonArray jsonArray;
    for (const auto &r: wallets) {
        jsonArray.push_back(r.first);
    }
    QJsonDocument json(jsonArray);
    return json.toJson(QJsonDocument::Compact);
}

static QString makeJsonWalletsInfo(const std::vector<wallets::WalletInfo> &wallets)
{
    QJsonArray jsonArray;
    for (const auto &r: wallets) {
        QJsonObject val;
        val.insert("address", r.address);
        if (r.type == wallets::WalletInfo::Type::Key)
            val.insert("type", 1);
        else if (r.type == wallets::WalletInfo::Type::Watch)
            val.insert("type", 2);
        else
            val.insert("type", -1);
        val.insert("path", r.path);
        jsonArray.push_back(val);
    }
    QJsonDocument json(jsonArray);
    return json.toJson(QJsonDocument::Compact);
}

static QString makeJsonWalletsAndPaths(const std::vector<std::pair<QString, QString>> &wallets) {
    QJsonArray jsonArray;
    for (const auto &r: wallets) {
        QJsonObject val;
        val.insert("address", r.first);
        val.insert("path", r.second);
        jsonArray.push_back(val);
    }
    QJsonDocument json(jsonArray);
    return json.toJson(QJsonDocument::Compact);
}

JavascriptWrapper::JavascriptWrapper(
    MainWindow &mainWindow,
    WebSocketClient &wssClient,
    NsLookup &nsLookup,
    transactions::Transactions &transactionsManager,
    auth::Auth &authManager,
    NetwrokTesting &networkTesting,
    utils::Utils &utilsManager,
    wallets::Wallets &wallets,
    const QString &applicationVersion,
    QObject */*parent*/
)
    : walletDefaultPath(getWalletPath())
    , mainWindow(mainWindow)
    , wssClient(wssClient)
    , nsLookup(nsLookup)
    , transactionsManager(transactionsManager)
    , networkTesting(networkTesting)
    , utilsManager(utilsManager)
    , wallets(wallets)
    , applicationVersion(applicationVersion)
    , localServer("metagate_proxy")
{
    hardwareId = QString::fromStdString(::getMachineUid());
    utmData = QString::fromLatin1(getUtmData());

    LOG << "Wallets default path " << walletDefaultPath;

    Q_CONNECT(&client, &SimpleClient::callbackCall, this, &JavascriptWrapper::onCallbackCall);
    Q_CONNECT(this, &JavascriptWrapper::callbackCall, this, &JavascriptWrapper::onCallbackCall);

    Q_CONNECT(&fileSystemWatcher, &QFileSystemWatcher::directoryChanged, this, &JavascriptWrapper::onDirChanged);

    Q_CONNECT(&wssClient, &WebSocketClient::messageReceived, this, &JavascriptWrapper::onWssMessageReceived);

    Q_CONNECT(this, &JavascriptWrapper::sendCommandLineMessageToWssSig, this, &JavascriptWrapper::onSendCommandLineMessageToWss);

    Q_CONNECT(&authManager, &auth::Auth::logined2, this, &JavascriptWrapper::onLogined);

    Q_CONNECT(&wallets, &wallets::Wallets::watchWalletsAdded, this, &JavascriptWrapper::onWatchWalletsAdded);

    Q_REG2(TypedException, "TypedException", false);
    Q_REG(JavascriptWrapper::ReturnCallback, "JavascriptWrapper::ReturnCallback");
    Q_REG(JavascriptWrapper::WalletCurrency, "JavascriptWrapper::WalletCurrency");

    signalFunc = std::bind(&JavascriptWrapper::callbackCall, this, _1);

    Q_CONNECT3(&localServer, &LocalServer::request, [](std::shared_ptr<LocalServerRequest> request) {
        LOG << "Request: " << request->request();
        request->response("Ya tuta");
    });

    emit authManager.reEmit();
}

void JavascriptWrapper::onWatchWalletsAdded(bool /*isMhc*/, const std::vector<std::pair<QString, QString>> &/*created*/) {
BEGIN_SLOT_WRAPPER
    sendAppInfoToWss(userName, true);
END_SLOT_WRAPPER
}

void JavascriptWrapper::mvToThread(QThread *thread) {
    moveToThread(thread);
    client.moveToThread(thread);
    fileSystemWatcher.moveToThread(thread);
    localServer.mvToThread(thread);
}

void JavascriptWrapper::onCallbackCall(ReturnCallback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void JavascriptWrapper::setWidget(QWidget *widget) {
    widget_ = widget;
}

void JavascriptWrapper::onLogined(bool /*isInit*/, const QString &login, const QString &token_) {
BEGIN_SLOT_WRAPPER
    if (!login.isEmpty()) {
        setPathsImpl(makePath(walletDefaultPath, login), login);
        token = token_;
    } else {
        setPathsImpl(makePath(walletDefaultPath, defaultUsername), defaultUsername);
    }
END_SLOT_WRAPPER
}

template<typename... Args>
void JavascriptWrapper::makeAndRunJsFuncParams(const QString &function, const QString &lastArg, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc2<true>(function, lastArg, exception, std::forward<Args>(args)...);
    runJs(res);
}

template<typename... Args>
void JavascriptWrapper::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc2<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void JavascriptWrapper::sendAppInfoToWss(QString userName, bool force) {
    const QString newUserName = userName;
    if (force || newUserName != sendedUserName) {
        std::vector<QString> keysTmh;
        CHECK(!walletPath.isEmpty(), "Wallet paths is empty");
        const std::vector<std::pair<QString, QString>> keys1 = Wallet::getAllWalletsInFolder(walletPath, false, true);
        std::transform(keys1.begin(), keys1.end(), std::back_inserter(keysTmh), [](const auto &pair) {return pair.first;});
        std::vector<QString> keysMth;
        const std::vector<std::pair<QString, QString>> keys2 = Wallet::getAllWalletsInFolder(walletPath, true, true);
        std::transform(keys2.begin(), keys2.end(), std::back_inserter(keysMth), [](const auto &pair) {return pair.first;});

        QSettings settings(getRuntimeSettingsPath(), QSettings::IniFormat);
        const bool isForgingActive = settings.value("forging/enabled", true).toBool();
        const QString message = makeMessageApplicationForWss(hardwareId, utmData, newUserName, applicationVersion, mainWindow.getCurrentHtmls().lastVersion, isForgingActive, keysTmh, keysMth);
        LOG << "Send MetaGate info to wss. Count keys " << keysTmh.size() << " " << keysMth.size() << ". " << userName;
        emit wssClient.sendMessage(message);
        emit wssClient.setHelloString(message, "jsWrapper");
        sendedUserName = newUserName;
    }
}

QByteArray JavascriptWrapper::getUtmData() {
    QDir dir(qApp->applicationDirPath());

    QFile file(dir.filePath(QStringLiteral("installer.ins")));
    if (!file.open(QIODevice::ReadOnly))
        return QByteArray();
    QByteArray data = file.read(1024);
    file.close();
    return data;
}

void JavascriptWrapper::onSendCommandLineMessageToWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText) {
BEGIN_SLOT_WRAPPER
    LOG << "Send command line " << line;
    emit wssClient.sendMessage(makeCommandLineMessageForWss(hardwareId, userId, focusCount, line, isEnter, isUserText));
END_SLOT_WRAPPER
}

////////////////
/// METAHASH ///
////////////////

void JavascriptWrapper::createWalletMTHS(QString requestId, QString password, bool isMhc, QString jsNameResult) {
    LOG << "Create wallet mths " << requestId << " " << walletPath;
    wallets.createWallet(isMhc, password, wallets::Wallets::CreateWalletCallback([this, jsNameResult, requestId](const QString &fullPath, const std::string &pubkey, const std::string &address, const std::string &exampleMessage, const std::string &sign) {
        sendAppInfoToWss(userName, true);
        LOG << "Create wallet mths ok " << requestId << " " << walletPath << " " << address;
        makeAndRunJsFuncParams(jsNameResult, fullPath, TypedException(), Opt<QString>(requestId), Opt<std::string>(pubkey), Opt<std::string>(address), Opt<std::string>(exampleMessage), Opt<std::string>(sign));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, "", e, Opt<QString>(requestId), Opt<std::string>(""), Opt<std::string>(""), Opt<std::string>(""), Opt<std::string>(""));
    }, signalFunc));
}

void JavascriptWrapper::createWalletMTHSWatch(QString requestId, QString address, QString jsNameResult, bool isMhc)
{
    LOG << "Create wallet mths watch " << requestId << " " << address;
    wallets.createWatchWallet(isMhc, address, wallets::Wallets::CreateWatchWalletCallback([this, jsNameResult, requestId, address](const QString &fullPath) {
        sendAppInfoToWss(userName, true);
        LOG << "Create wallet mths watch ok " << requestId << " " << walletPath << " " << address;
        makeAndRunJsFuncParams(jsNameResult, fullPath, TypedException(), Opt<QString>(requestId), Opt<QString>(address));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, "", e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::removeWalletMTHSWatch(QString requestId, QString address, QString jsNameResult, bool isMhc) {
    LOG << "Remove wallet mths watch " << requestId << " " << address;
    wallets.removeWatchWallet(isMhc, address, wallets::Wallets::RemoveWatchWalletCallback([this, jsNameResult, requestId, address]() {
        sendAppInfoToWss(userName, true);
        LOG << "Remove wallet mths watch ok " << requestId << " " << address;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(address));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::checkWalletMTHSExists(QString requestId, QString address, bool isMhc, QString jsNameResult) {
    wallets.checkWalletExist(isMhc, address, wallets::Wallets::CheckWalletExistCallback([this, jsNameResult, requestId, address](bool isExist) {
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<bool>(isExist));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<bool>(false));
    }, signalFunc));
}

void JavascriptWrapper::checkWalletPasswordMTHS(QString requestId, QString keyName, QString password, bool isMhc, QString jsNameResult) {
    LOG << "Check wallet password " << requestId << " " << keyName << " " << walletPath;

    wallets.checkWalletPassword(isMhc, keyName, password, wallets::Wallets::CheckWalletPasswordCallback([this, jsNameResult, requestId, keyName](bool result) {
        LOG << "Check wallet password ok " << requestId << " " << keyName;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(result ? "Ok" : "Not ok"));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>("Not ok"));
    }, signalFunc));
}

void JavascriptWrapper::createWallet(QString requestId, QString password) {
BEGIN_SLOT_WRAPPER
    createWalletMTHS(requestId, password, false, "createWalletResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::createWalletWatch(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    createWalletMTHSWatch(requestId, address, "createWalletWatchResultJs", false);
END_SLOT_WRAPPER
}

void JavascriptWrapper::checkWalletExists(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    checkWalletMTHSExists(requestId, address, false, "checkWalletExistsResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::checkWalletPassword(QString requestId, QString keyName, QString password) {
BEGIN_SLOT_WRAPPER
    checkWalletPasswordMTHS(requestId, keyName, password, false, "checkWalletPasswordResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::removeWalletWatch(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    removeWalletMTHSWatch(requestId, address, "removeWalletWatchResultJs", false);
END_SLOT_WRAPPER
}

void JavascriptWrapper::createWalletMHC(QString requestId, QString password) {
BEGIN_SLOT_WRAPPER
    createWalletMTHS(requestId, password, true, "createWalletMHCResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::createWalletWatchMHC(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    createWalletMTHSWatch(requestId, address, "createWalletWatchMHCResultJs", true);
END_SLOT_WRAPPER
}

void JavascriptWrapper::checkWalletExistsMHC(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    checkWalletMTHSExists(requestId, address, true, "checkWalletExistsMHCResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::checkWalletPasswordMHC(QString requestId, QString keyName, QString password) {
BEGIN_SLOT_WRAPPER
    checkWalletPasswordMTHS(requestId, keyName, password, true, "checkWalletPasswordMHCResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::removeWalletWatchMHC(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    removeWalletMTHSWatch(requestId, address, "removeWalletWatchMHCResultJs", true);
END_SLOT_WRAPPER
}

QString JavascriptWrapper::getAllWalletsJson() {
    return getAllMTHSWalletsJson(false, "tmh");
}

QString JavascriptWrapper::getAllWalletsInfoJson() {
    return getAllMTHSWalletsInfoJson(false, "tmh");
}

QString JavascriptWrapper::getAllMHCWalletsJson() {
    return getAllMTHSWalletsJson(true, "mhc");
}

QString JavascriptWrapper::getAllMHCWalletsInfoJson() {
    return getAllMTHSWalletsInfoJson(true, "mhc");
}

QString JavascriptWrapper::getAllWalletsAndPathsJson() {
    return getAllMTHSWalletsAndPathsJson(false, "tmh");
}

QString JavascriptWrapper::getAllMHCWalletsAndPathsJson() {
    return getAllMTHSWalletsAndPathsJson(true, "mhc");
}

void JavascriptWrapper::signMessage(QString requestId, QString keyName, QString text, QString password) {
BEGIN_SLOT_WRAPPER
    signMessageMTHS(requestId, keyName, text, password, false, "signMessageResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex) {
BEGIN_SLOT_WRAPPER
    signMessageMTHS(requestId, keyName, password, toAddress, value, fee, nonce, dataHex, false, "signMessageV2ResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageMTHSV3(requestId, keyName, password, toAddress, value, fee, nonce, dataHex, paramsJson, false, "signMessageV3ResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageDelegateMTHS(requestId, keyName, password, toAddress, value, fee, nonce, valueDelegate, true, paramsJson, false, "signMessageDelegateResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageUnDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageDelegateMTHS(requestId, keyName, password, toAddress, value, fee, nonce, valueDelegate, false, paramsJson, false, "signMessageUnDelegateResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHCDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageDelegateMTHS(requestId, keyName, password, toAddress, value, fee, nonce, valueDelegate, true, paramsJson, true, "signMessageDelegateMhcResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHCUnDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageDelegateMTHS(requestId, keyName, password, toAddress, value, fee, nonce, valueDelegate, false, paramsJson, true, "signMessageUnDelegateMhcResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHC(QString requestId, QString keyName, QString text, QString password) {
BEGIN_SLOT_WRAPPER
    signMessageMTHS(requestId, keyName, text, password, true, "signMessageMHCResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHCV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex) {
BEGIN_SLOT_WRAPPER
    signMessageMTHS(requestId, keyName, password, toAddress, value, fee, nonce, dataHex, true, "signMessageMHCV2ResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHCV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageMTHSV3(requestId, keyName, password, toAddress, value, fee, nonce, dataHex, paramsJson, true, "signMessageMHCV3ResultJs");
END_SLOT_WRAPPER
}

QString JavascriptWrapper::getAllMTHSWalletsAndPathsJson(bool isMhc, QString name) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPath, isMhc);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << PeriodicLog::make("w2_" + name.toStdString()) << "get " << name << " wallets json " << jsonStr << " " << walletPath;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString JavascriptWrapper::getAllMTHSWalletsJson(bool isMhc, QString name) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPath, isMhc);
        const QString jsonStr = makeJsonWallets(result);
        LOG << PeriodicLog::make("w_" + name.toStdString()) << "get " << name << " wallets json " << jsonStr << " " << walletPath;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString JavascriptWrapper::getAllMTHSWalletsInfoJson(bool isMhc, QString name) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<wallets::WalletInfo> result = Wallet::getAllWalletsInfoInFolder(walletPath, isMhc);
        const QString jsonStr = makeJsonWalletsInfo(result);
        LOG << PeriodicLog::make("w3_" + name.toStdString()) << "get " << name << " wallets json " << jsonStr << " " << walletPath;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

void JavascriptWrapper::checkAddress(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    LOG << "Check address " << address;
    const QString JS_NAME_RESULT = "checkAddressResultJs";

    wallets.checkAddress(address, wallets::Wallets::CheckAddressCallback([this, JS_NAME_RESULT, requestId, address](bool result) {
        LOG << "Check address ok " << requestId << " " << address;
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result ? "ok" : "not valid"));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>("not valid"));
    }, signalFunc));
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString text, QString password, bool isMhc, QString jsNameResult) {
    LOG << "Sign message " << requestId << " " << keyName << " " << isMhc << " " << text;

    wallets.signMessage(isMhc, keyName, text, password, wallets::Wallets::SignMessageCallback([this, jsNameResult, requestId, keyName](const QString &signature, const QString &pubkey) {
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(signature), Opt<QString>(pubkey));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, bool isMhc, QString jsNameResult) {
    LOG << "Sign message " << requestId << " " << keyName << " " << isMhc << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << dataHex;

    wallets.signMessage2(isMhc, keyName, password, toAddress, value, fee, nonce, dataHex, wallets::Wallets::SignMessage2Callback([this, jsNameResult, requestId, keyName](const QString &signature, const QString &pubkey, const QString &tx) {
        LOG << "Sign message ok " << Wallet::calcHash(tx.toStdString(), signature.toStdString(), pubkey.toStdString());

        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(signature), Opt<QString>(pubkey), Opt<QString>(tx));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""), Opt<QString>(""), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::createV8AddressImpl(QString requestId, const QString jsNameResult, QString address, int nonce) {
    wallets.createContractAddress(address, nonce, wallets::Wallets::CreateContractAddressCallback([this, jsNameResult, requestId, address](const QString &result) {
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::signMessageMTHSV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson, bool isMhc, QString jsNameResult) {
    LOG << "Sign messagev3 " << requestId << " " << keyName << " " << isMhc << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << dataHex;

    wallets.signAndSendMessage(isMhc, keyName, password, toAddress, value, fee, nonce, dataHex, paramsJson, wallets::Wallets::SignAndSendMessageCallback([this, jsNameResult, requestId, keyName](bool /*success*/) {
        LOG << "Sign messagev3 ok " << keyName;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>("Ok"));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>("Not ok"));
    }, signalFunc));
}

void JavascriptWrapper::signMessageDelegateMTHS(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, bool isDelegate, QString paramsJson, bool isMhc, QString jsNameResult) {
    LOG << "Sign message delegate " << requestId << " " << keyName << " " << isMhc << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << "is_delegate: " << (isDelegate ? "true" : "false") << " " << valueDelegate;

    wallets.signAndSendMessageDelegate(isMhc, keyName, password, toAddress, value, fee, valueDelegate, nonce, isDelegate, paramsJson, wallets::Wallets::SignAndSendMessageCallback([this, jsNameResult, requestId, keyName](bool /*success*/) {
        LOG << "Sign messagev delegate ok " << keyName;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>("Ok"));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>("Not ok"));
    }, signalFunc));
}

void JavascriptWrapper::getOnePrivateKeyMTHS(QString requestId, QString keyName, bool isCompact, QString jsNameResult, bool isMhc) {
    wallets.getOnePrivateKey(isMhc, keyName, isCompact, wallets::Wallets::GetPrivateKeyCallback([this, jsNameResult, requestId, keyName](const QString &result) {
        LOG << "Getted private key " << keyName;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::getOnePrivateKey(QString requestId, QString keyName, bool isCompact) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getOnePrivateKeyResultJs";
    getOnePrivateKeyMTHS(requestId, keyName, isCompact, JS_NAME_RESULT, false);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getOnePrivateKeyMHC(QString requestId, QString keyName, bool isCompact) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getOnePrivateKeyMHCResultJs";
    getOnePrivateKeyMTHS(requestId, keyName, isCompact, JS_NAME_RESULT, true);
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyMTHS(QString requestId, QString privateKey, QString password, bool isMhc, QString jsNameResult) {
    wallets.savePrivateKey(isMhc, privateKey, password, wallets::Wallets::SavePrivateKeyCallback([this, jsNameResult, requestId](bool /*success*/, const QString &/*address*/) {
        LOG << "Save private key";
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>("ok"));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>("Not ok"));
    }, signalFunc));
}

void JavascriptWrapper::savePrivateKey(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "savePrivateKeyResultJs";
    savePrivateKeyMTHS(requestId, privateKey, password, false, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyMHC(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "savePrivateKeyMHCResultJs";
    savePrivateKeyMTHS(requestId, privateKey, password, true, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::saveRawPrivKeyMTHS(QString requestId, QString rawPrivKey, QString password, bool isMhc, QString jsNameResult) {
    wallets.saveRawPrivateKey(isMhc, rawPrivKey, password, wallets::Wallets::SaveRawPrivateKeyCallback([this, jsNameResult, requestId](const QString &address) {
        LOG << "Save raw private key " << address;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(address));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::saveRawPrivKey(QString requestId, QString rawPrivKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "saveRawPrivkeyResultJs";
    saveRawPrivKeyMTHS(requestId, rawPrivKey, password, false, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::saveRawPrivKeyMHC(QString requestId, QString rawPrivKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "saveRawPrivkeyMHCResultJs";
    saveRawPrivKeyMTHS(requestId, rawPrivKey, password, true, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getRawPrivKeyMTHS(QString requestId, QString address, QString password, bool isMhc, QString jsNameResult) {
    wallets.getRawPrivateKey(isMhc, address, password, wallets::Wallets::GetRawPrivateKeyCallback([this, jsNameResult, requestId](const QString &result) {
        LOG << "Get raw private key ";
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::getRawPrivKey(QString requestId, QString address, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getRawPrivkeyResultJs";
    getRawPrivKeyMTHS(requestId, address, password, false, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getRawPrivKeyMHC(QString requestId, QString address, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getRawPrivkeyMHCResultJs";
    getRawPrivKeyMTHS(requestId, address, password, true, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::createV8AddressMHC(QString requestId, QString address, int nonce) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "createV8AddressMHCResultJs";
    createV8AddressImpl(requestId, JS_NAME_RESULT, address, nonce);
END_SLOT_WRAPPER
}

void JavascriptWrapper::createV8Address(QString requestId, QString address, int nonce) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "createV8AddressResultJs";
    createV8AddressImpl(requestId, JS_NAME_RESULT, address, nonce);
END_SLOT_WRAPPER
}

void JavascriptWrapper::createRsaKeyMTHS(QString requestId, QString address, QString password, bool isMhc, QString jsNameResult) {
    wallets.createRsaKey(isMhc, address, password, wallets::Wallets::GetRawPrivateKeyCallback([this, jsNameResult, requestId, address](const QString &pubkey) {
        LOG << "Create rsa key " << address;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(pubkey));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::getRsaPublicKeyMTHS(QString requestId, QString address, bool isMhc, QString jsNameResult) {
    wallets.getRsaPublicKey(isMhc, address, wallets::Wallets::GetRsaPublicKeyCallback([this, jsNameResult, requestId, address](const QString &pubkey) {
        LOG << "Get rsa public key " << address;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>(pubkey));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
}

void JavascriptWrapper::copyRsaKeyMTHS(QString requestId, QString address, QString pathPub, QString pathPriv, bool isMhc, QString jsNameResult) {
    wallets.copyRsaKey(isMhc, address, pathPub, pathPriv, wallets::Wallets::CopyRsaKeyCallback([this, jsNameResult, requestId, address](bool /*result*/) {
        LOG << "Copy rsa key " << address;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>("Ok"));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>("Not ok"));
    }, signalFunc));
}

void JavascriptWrapper::copyRsaKeyToFolderMTHS(QString requestId, QString address, QString path, bool isMhc, QString jsNameResult) {
    wallets.copyRsaKeyToFolder(isMhc, address, path, wallets::Wallets::CopyRsaKeyCallback([this, jsNameResult, requestId, address](bool /*result*/) {
        LOG << "Copy rsa key to folder " << address;
        makeAndRunJsFuncParams(jsNameResult, TypedException(), Opt<QString>(requestId), Opt<QString>("Ok"));
    }, [this, jsNameResult, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(jsNameResult, e, Opt<QString>(requestId), Opt<QString>("Not ok"));
    }, signalFunc));
}

void JavascriptWrapper::createRsaKey(QString requestId, QString address, QString password) {
BEGIN_SLOT_WRAPPER
    LOG << "Create rsa key tmh " << address;

    const QString JS_NAME_RESULT = "createRsaKeyResultJs";
    createRsaKeyMTHS(requestId, address, password, false, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getRsaPublicKey(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    LOG << "Get rsa key tmh " << address;

    const QString JS_NAME_RESULT = "getRsaPublicKeyResultJs";
    getRsaPublicKeyMTHS(requestId, address, false, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::createRsaKeyMHC(QString requestId, QString address, QString password) {
BEGIN_SLOT_WRAPPER
    LOG << "Create rsa key mhc " << address;

    const QString JS_NAME_RESULT = "createRsaKeyMHCResultJs";
    createRsaKeyMTHS(requestId, address, password, true, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getRsaPublicKeyMHC(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    LOG << "Get rsa key mhc " << address;

    const QString JS_NAME_RESULT = "getRsaPublicKeyMHCResultJs";
    getRsaPublicKeyMTHS(requestId, address, true, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::copyRsaKey(QString requestId, QString address, QString pathPub, QString pathPriv) {
BEGIN_SLOT_WRAPPER
    LOG << "copy rsa key tmh " << address;

    const QString JS_NAME_RESULT = "copyRsaKeyResultJs";
    copyRsaKeyMTHS(requestId, address, pathPub, pathPriv, false, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::copyRsaKeyMHC(QString requestId, QString address, QString pathPub, QString pathPriv) {
BEGIN_SLOT_WRAPPER
    LOG << "copy rsa key mhc " << address;

    const QString JS_NAME_RESULT = "copyRsaKeyMHCResultJs";
    copyRsaKeyMTHS(requestId, address, pathPub, pathPriv, true, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::copyRsaKeyToFolder(QString requestId, QString address, QString path) {
BEGIN_SLOT_WRAPPER
    LOG << "copy rsa key to folder tmh " << address << " " << path;

    const QString JS_NAME_RESULT = "copyRsaKeyToFolderResultJs";
    copyRsaKeyToFolderMTHS(requestId, address, path, false, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::copyRsaKeyToFolderMHC(QString requestId, QString address, QString path) {
BEGIN_SLOT_WRAPPER
    LOG << "copy rsa key to folder mhc " << address << " " << path;

    const QString JS_NAME_RESULT = "copyRsaKeyToFolderMHCResultJs";
    copyRsaKeyToFolderMTHS(requestId, address, path, true, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::encryptMessage(QString requestId, QString publicKey, QString message) {
BEGIN_SLOT_WRAPPER
    LOG << "encrypt message";

    const QString JS_NAME_RESULT = "encryptMessageResultJs";
    Opt<std::string> answer;
    const TypedException exception = apiVrapper2([&]() {
        const WalletRsa wallet = WalletRsa::fromPublicKey(publicKey.toStdString());
        answer = wallet.encrypt(message.toStdString());
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), answer);
END_SLOT_WRAPPER
}

void JavascriptWrapper::decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex) {
BEGIN_SLOT_WRAPPER
    LOG << "decrypt message " << addr;

    const QString JS_NAME_RESULT = "decryptMessageResultJs";
    Opt<std::string> message;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        WalletRsa wallet(walletPath, true, addr.toStdString());
        wallet.unlock(password.toStdString());
        message = wallet.decryptMessage(encryptedMessageHex.toStdString());
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), message);
END_SLOT_WRAPPER
}

////////////////
/// ETHEREUM ///
////////////////

void JavascriptWrapper::createWalletEth(QString requestId, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "createWalletEthResultJs";

    LOG << "Create wallet eth " << requestId;

    wallets.createEthKey(password, wallets::Wallets::CreateEthKeyCallback([this, JS_NAME_RESULT, requestId](const QString &address, const QString &fullPath) {
        LOG << "Create eth key ok " << address;
        makeAndRunJsFuncParams(JS_NAME_RESULT, fullPath, TypedException(), Opt<QString>(requestId), Opt<QString>(address));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, "", e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "signMessageEthResultJs";

    LOG << "Sign message eth " << address << " " << nonce << " " << gasPrice << " " << gasLimit << " " << to << " " << value << " " << data;

    wallets.signMessageEth(address, password, nonce, gasPrice, gasLimit, to, value, data, wallets::Wallets::SignMessageEthCallback([this, JS_NAME_RESULT, requestId](const QString &result) {
        LOG << "Sign message eth ok ";
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
END_SLOT_WRAPPER
}

void JavascriptWrapper::checkAddressEth(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    LOG << "Check address eth " << address;
    const QString JS_NAME_RESULT = "checkAddressEthResultJs";

    wallets.checkAddressEth(address, wallets::Wallets::CheckAddressCallback([this, JS_NAME_RESULT, requestId](bool result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result ? "ok" : "not valid"));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>("not valid"));
    }, signalFunc));
END_SLOT_WRAPPER
}

/*void JavascriptWrapper::signMessageTokensEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString contractAddress, QString to, QString value) {
    const QString JS_NAME_RESULT = "signMessageEthResultJs";

    LOG << "Sign message tokens eth" << std::endl;

    const TypedException &exception = apiVrapper([&, this]() {
        const QString data = QString::fromStdString(EthWallet::makeErc20Data(value.toStdString(), to.toStdString()));
        signMessageEth(requestId, address, password, nonce, gasPrice, gasLimit, contractAddress, "0x0", data);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}*/

QString JavascriptWrapper::getAllEthWalletsJson() {
    try {
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = EthWallet::getAllWalletsInFolder(walletPath);
        const QString jsonStr = makeJsonWallets(result);
        LOG << PeriodicLog::make("w_eth") << "get eth wallets json " << jsonStr;
        return jsonStr;
    } catch (const TypedException &e) {
        LOG << "Error: " + e.description;
        return "Error: " + QString::fromStdString(e.description);
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (const std::exception &e) {
        LOG << "Error: " << e.what();
        return "Error: " + QString::fromStdString(e.what());
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString JavascriptWrapper::getAllEthWalletsAndPathsJson() {
    try {
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = EthWallet::getAllWalletsInFolder(walletPath);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << PeriodicLog::make("w2_eth") << "get eth wallets json " << jsonStr;
        return jsonStr;
    } catch (const TypedException &e) {
        LOG << "Error: " + e.description;
        return "Error: " + QString::fromStdString(e.description);
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (const std::exception &e) {
        LOG << "Error: " << e.what();
        return "Error: " + QString::fromStdString(e.what());
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

void JavascriptWrapper::getOnePrivateKeyEth(QString requestId, QString keyName) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getOnePrivateKeyEthResultJs";

    LOG << "get one private key eth " << keyName;

    wallets.getOnePrivateKeyEth(keyName, wallets::Wallets::GetPrivateKeyCallback([this, JS_NAME_RESULT, requestId, keyName](const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyEth(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "savePrivateKeyEthResultJs";

    wallets.savePrivateKeyEth(privateKey, password, wallets::Wallets::SavePrivateKeyCallback([this, JS_NAME_RESULT, requestId](bool result, const QString &/*address*/) {
        LOG << "Save private key eth";
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result ? "ok" : "Not ok"));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>("not valid"));
    }, signalFunc));
END_SLOT_WRAPPER
}

///////////////
/// BITCOIN ///
///////////////

void JavascriptWrapper::createWalletBtcPswd(QString requestId, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "createWalletBtcResultJs";

    LOG << "Create wallet btc " << requestId;

    wallets.createBtcKey(password, wallets::Wallets::CreateBtcKeyCallback([this, JS_NAME_RESULT, requestId](const QString &address, const QString &fullPath) {
        LOG << "Create btc wallet ok " << requestId << " " << address;
        makeAndRunJsFuncParams(JS_NAME_RESULT, fullPath, TypedException(), Opt<QString>(requestId), Opt<QString>(address));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, "", e, Opt<QString>(requestId), Opt<QString>("not valid"));
    }, signalFunc));
END_SLOT_WRAPPER
}

void JavascriptWrapper::createWalletBtc(QString requestId) {
BEGIN_SLOT_WRAPPER
    createWalletBtcPswd(requestId, "");
END_SLOT_WRAPPER
}

void JavascriptWrapper::checkAddressBtc(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    LOG << "Check address btc " << address;
    const QString JS_NAME_RESULT = "checkAddressBtcResultJs";
    wallets.checkAddressBtc(address, wallets::Wallets::CheckAddressCallback([this, JS_NAME_RESULT, requestId](bool result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result ? "ok" : "not valid"));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>("not valid"));
    }, signalFunc));
END_SLOT_WRAPPER
}

// deprecated
void JavascriptWrapper::signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "signMessageBtcResultJs";

    LOG << "Sign message btc " << address << " " << toAddress << " " << value << " " << estimateComissionInSatoshi << " " << fees;

    const TypedException exception = apiVrapper2([&]{
        const QJsonDocument document = QJsonDocument::fromJson(jsonInputs.toUtf8());
        CHECK(document.isArray(), "jsonInputs not array");
        const QJsonArray root = document.array();
        std::vector<BtcInput> btcInputs;
        for (const auto &jsonObj2: root) {
            const QJsonObject jsonObj = jsonObj2.toObject();
            BtcInput input;
            CHECK(jsonObj.contains("value") && jsonObj.value("value").isString(), "value field not found");
            input.outBalance = std::stoull(jsonObj.value("value").toString().toStdString());
            CHECK(jsonObj.contains("scriptPubKey") && jsonObj.value("scriptPubKey").isString(), "scriptPubKey field not found");
            input.scriptPubkey = jsonObj.value("scriptPubKey").toString().toStdString();
            CHECK(jsonObj.contains("tx_index") && jsonObj.value("tx_index").isDouble(), "tx_index field not found");
            input.spendoutnum = jsonObj.value("tx_index").toInt();
            CHECK(jsonObj.contains("tx_hash") && jsonObj.value("tx_hash").isString(), "tx_hash field not found");
            input.spendtxid = jsonObj.value("tx_hash").toString().toStdString();
            btcInputs.emplace_back(input);
        }

        wallets.signMessageBtcUsedUtxos(address, password, btcInputs, toAddress, value, estimateComissionInSatoshi, fees, std::set<std::string>(), wallets::Wallets::SignMessageBtcCallback([this, JS_NAME_RESULT, requestId](const QString &result, const QString &/*hash*/, const std::set<std::string> &/*usedUtxos*/) {
            LOG << "Sign message btc ok";
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
        }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
            makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>(""));
        }, signalFunc));
    });
    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), Opt<QString>(""));
    }
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageBtcPswdUsedUtxos(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees, QString jsonUsedUtxos) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "signMessageBtcUsedUtxosResultJs";

    LOG << "Sign message btc utxos " << address << " " << toAddress << " " << value << " " << estimateComissionInSatoshi << " " << fees;

    const TypedException exception = apiVrapper2([&]{
        std::set<std::string> usedUtxos;
        const QJsonDocument documentUsed = QJsonDocument::fromJson(jsonUsedUtxos.toUtf8());
        CHECK(documentUsed.isArray(), "jsonInputs not array");
        const QJsonArray rootUsed = documentUsed.array();
        for (const auto &jsonUsedUtxo: rootUsed) {
            CHECK(jsonUsedUtxo.isString(), "value field not found");
            usedUtxos.insert(jsonUsedUtxo.toString().toStdString());
        }
        LOG << "Used utxos: " << usedUtxos.size();

        const QJsonDocument document = QJsonDocument::fromJson(jsonInputs.toUtf8());
        CHECK(document.isArray(), "jsonInputs not array");
        const QJsonArray root = document.array();
        std::vector<BtcInput> btcInputs;
        for (const auto &jsonObj2: root) {
            const QJsonObject jsonObj = jsonObj2.toObject();
            BtcInput input;
            CHECK(jsonObj.contains("value") && jsonObj.value("value").isString(), "value field not found");
            input.outBalance = std::stoull(jsonObj.value("value").toString().toStdString());
            CHECK(jsonObj.contains("scriptPubKey") && jsonObj.value("scriptPubKey").isString(), "scriptPubKey field not found");
            input.scriptPubkey = jsonObj.value("scriptPubKey").toString().toStdString();
            CHECK(jsonObj.contains("tx_index") && jsonObj.value("tx_index").isDouble(), "tx_index field not found");
            input.spendoutnum = jsonObj.value("tx_index").toInt();
            CHECK(jsonObj.contains("tx_hash") && jsonObj.value("tx_hash").isString(), "tx_hash field not found");
            input.spendtxid = jsonObj.value("tx_hash").toString().toStdString();
            btcInputs.emplace_back(input);
        }

        wallets.signMessageBtcUsedUtxos(address, password, btcInputs, toAddress, value, estimateComissionInSatoshi, fees, usedUtxos, wallets::Wallets::SignMessageBtcCallback([this, JS_NAME_RESULT, requestId](const QString &result, const QString &hash, const std::set<std::string> &usedUtxos) {
            LOG << "Sign message btc utxos ok";
            QJsonArray jsonArrayUtxos;
            for (const std::string &r: usedUtxos) {
                jsonArrayUtxos.push_back(QString::fromStdString(r));
            }
            const QJsonDocument jsonUtxos = QJsonDocument(jsonArrayUtxos);

            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result), Opt<QJsonDocument>(jsonUtxos), Opt<QString>(hash));
        }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
            makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>(""), Opt<QJsonDocument>(QJsonDocument()), Opt<QString>(""));
        }, signalFunc));
    });
    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), Opt<QString>(""), Opt<QJsonDocument>(QJsonDocument()), Opt<QString>(""));
    }
END_SLOT_WRAPPER
}

// deprecated
void JavascriptWrapper::signMessageBtc(QString requestId, QString address, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
BEGIN_SLOT_WRAPPER
    signMessageBtcPswd(requestId, address, "", jsonInputs, toAddress, value, estimateComissionInSatoshi, fees);
END_SLOT_WRAPPER
}

QString JavascriptWrapper::getAllBtcWalletsJson() {
    try {
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = BtcWallet::getAllWalletsInFolder(walletPath);
        const QString jsonStr = makeJsonWallets(result);
        LOG << PeriodicLog::make("w_btc") << "get btc wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString JavascriptWrapper::getAllBtcWalletsAndPathsJson() {
    try {
        CHECK(!walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = BtcWallet::getAllWalletsInFolder(walletPath);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << PeriodicLog::make("w2_btc") << "get btc wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

void JavascriptWrapper::getOnePrivateKeyBtc(QString requestId, QString keyName) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getOnePrivateKeyBtcResultJs";

    LOG << "get one private key btc " << keyName;

    wallets.getOnePrivateKeyBtc(keyName, wallets::Wallets::GetPrivateKeyCallback([this, JS_NAME_RESULT, requestId, keyName](const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>(""));
    }, signalFunc));
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyBtc(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "savePrivateKeyBtcResultJs";

    wallets.savePrivateKeyBtc(privateKey, password, wallets::Wallets::SavePrivateKeyCallback([this, JS_NAME_RESULT, requestId](bool result, const QString &/*address*/) {
        LOG << "Save private key btc";
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result ? "ok" : "Not ok"));
    }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>("not valid"));
    }, signalFunc));
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyAny(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const std::string key = privateKey.toStdString();
    if (key.compare(0, Wallet::PREFIX_ONE_KEY_MTH.size(), Wallet::PREFIX_ONE_KEY_MTH) == 0) {
        savePrivateKeyMHC(requestId, privateKey, password);
    } else if (key.compare(0, Wallet::PREFIX_ONE_KEY_TMH.size(), Wallet::PREFIX_ONE_KEY_TMH) == 0) {
        savePrivateKey(requestId, privateKey, password);
    } else if (key.compare(0, EthWallet::PREFIX_ONE_KEY.size(), EthWallet::PREFIX_ONE_KEY) == 0) {
        savePrivateKeyEth(requestId, privateKey, password);
    } else { // Если это не btc, то ошибка поймается внутри этой функции
        savePrivateKeyBtc(requestId, privateKey, password);
    }
END_SLOT_WRAPPER
}

//////////////
/// COMMON ///
//////////////

void JavascriptWrapper::updateAndReloadApplication() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "reloadApplicationJs";

    LOG << "Reload application ";

    Opt<QString> result;
    const TypedException exception = apiVrapper2([&]() {
        updateAndRestart();
        result = "Ok";
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::qtOpenInBrowser(QString url) {
BEGIN_SLOT_WRAPPER
    LOG << "Open another url " << url;
    emit utilsManager.openInBrowser(url, utils::Utils::OpenInBrowserCallback([]{}, [](const TypedException &/*e*/) {}, std::bind(&JavascriptWrapper::callbackCall, this, _1)));
END_SLOT_WRAPPER
}

void JavascriptWrapper::getWalletFolders() {
BEGIN_SLOT_WRAPPER
    LOG << "getWalletFolders ";
    const QString JS_NAME_RESULT = "walletFoldersJs";

    wallets.getWalletFolders(wallets::Wallets::GetWalletFoldersCallback([this, JS_NAME_RESULT](const QString &defaultFolder, const QString &folder, const QString &userName) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(defaultFolder), Opt<QString>(folder), Opt<QString>(userName));
    }, [this, JS_NAME_RESULT](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(""), Opt<QString>(""), Opt<QString>(""));
    }, signalFunc));
END_SLOT_WRAPPER
}

bool JavascriptWrapper::migrateKeysToPath(QString /*newPath*/) {
    /*LOG << "Migrate keys to path " << newPath;

    const QString prevPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_PREV_PATH);

    copyRecursively(makePath(prevPath, WALLET_PATH_ETH), makePath(newPath, WALLET_PATH_ETH), false);
    copyRecursively(makePath(prevPath, WALLET_PATH_BTC), makePath(newPath, WALLET_PATH_BTC), false);
    copyRecursively(makePath(prevPath, Wallet::WALLET_PATH_MTH), makePath(newPath, Wallet::WALLET_PATH_MTH), false);
    copyRecursively(makePath(prevPath, WALLET_PATH_TMH), makePath(newPath, WALLET_PATH_TMH), false);
    copyRecursively(prevPath, makePath(newPath, WALLET_PATH_TMH), false);*/

    return true;
}

void JavascriptWrapper::setPathsImpl(QString newPatch, QString newUserName) {
    userName = newUserName;

    if (walletPath == newPatch) {
        return;
    }

    walletPath = newPatch;
    CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
    createFolder(walletPath);

    for (const FolderWalletInfo &folderInfo: folderWalletsInfos) {
        fileSystemWatcher.removePath(folderInfo.walletPath.absolutePath());
    }
    folderWalletsInfos.clear();

    auto setPathToWallet = [this](const QString &suffix, const QString &name) {
        const QString curPath = makePath(walletPath, suffix);
        createFolder(curPath);
        folderWalletsInfos.emplace_back(curPath, name);
        fileSystemWatcher.addPath(curPath);
    };

    setPathToWallet(EthWallet::subfolder(), "eth");
    setPathToWallet(BtcWallet::subfolder(), "btc");
    setPathToWallet(Wallet::chooseSubfolder(true), "mhc");
    setPathToWallet(Wallet::chooseSubfolder(false), "tmh");

    LOG << "Wallets path " << walletPath;

    sendAppInfoToWss(newUserName, false);
}

QString JavascriptWrapper::openFolderDialog(QString beginPath, QString caption) {
    const QString dir = QFileDialog::getExistingDirectory(widget_, caption, beginPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    LOG << "choised dir " << dir;
    return dir;
}

void JavascriptWrapper::exitApplication() {
BEGIN_SLOT_WRAPPER
    LOG << "Exit";
    QApplication::exit(SIMPLE_EXIT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::restartBrowser() {
BEGIN_SLOT_WRAPPER
    LOG << "Restart browser";
    QApplication::exit(RESTART_BROWSER);
END_SLOT_WRAPPER
}

QString JavascriptWrapper::backupKeys(QString caption) {
    try {
        LOG << "Backup keys";
        const QString beginPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), "backup.zip");
        const QString file = QFileDialog::getSaveFileName(widget_, caption, beginPath);
        LOG << "Backup keys to file " << file;
        ::backupKeys(walletPath, file);
        return "";
    } catch (const Exception &e) {
        return QString::fromStdString(e);
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "Unknown error";
    }
}

QString JavascriptWrapper::restoreKeys(QString caption) {
    try {
        LOG << "Restore keys";
        const QString beginPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), "backup.zip");
        const QString file = QFileDialog::getOpenFileName(widget_, caption, beginPath, "*.zip;;*.*");
        LOG << "Restore keys from file " << file;
        const std::string text = checkBackupFile(file);
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question(widget_, "caption", "Restore backup " + QString::fromStdString(text) + "?", QMessageBox::Yes|QMessageBox::No);
        if (reply == QMessageBox::Yes) {
            ::restoreKeys(file, walletPath);
        }
        return "";
    } catch (const Exception &e) {
        return QString::fromStdString(e);
    } catch (const std::exception &e) {
        return e.what();
    } catch (...) {
        return "Unknown error";
    }
}

void JavascriptWrapper::getMachineUid() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "machineUidJs";

    LOG << "Get machine uid " << hardwareId;

    const QString uid = "\"" + hardwareId + "\"";
    runJs(JS_NAME_RESULT + "(" +
        uid + "" +
        ");"
    );
END_SLOT_WRAPPER
}

void JavascriptWrapper::lineEditReturnPressed(QString text) {
BEGIN_SLOT_WRAPPER
    emit lineEditReturnPressedSig(text);
END_SLOT_WRAPPER
}

void JavascriptWrapper::openFolderInStandartExplored(const QString &folder) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void JavascriptWrapper::openWalletPathInStandartExplorer() {
BEGIN_SLOT_WRAPPER
    emit wallets.openWalletPathInStandartExplorer();
END_SLOT_WRAPPER
}

void JavascriptWrapper::getIpsServers(QString requestId, QString type, int length, int count) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getIpsServersJs";

    LOG << "get ips servers " << type << " " << length << " " << count;

    const TypedException exception = apiVrapper2([&, this]() {
        nsLookup.getRandomServersWithoutHttp(type, length, count, NsLookup::GetServersCallback([this, JS_NAME_RESULT, requestId, type](const std::vector<QString> &servers) {
            QString resultStr = "[";
            bool isFirst = true;
            for (const QString &r: servers) {
                if (!isFirst) {
                    resultStr += ", ";
                }
                isFirst = false;
                resultStr += "\"" + r + "\"";
            }
            resultStr += "]";

            LOG << "get ips servers ok " << type << " " << resultStr;

            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(resultStr));
        }, [this, JS_NAME_RESULT, requestId](const TypedException &e) {
            makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>(""));
        }, std::bind(&JavascriptWrapper::callbackCall, this, _1)));
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), Opt<QString>(""));
END_SLOT_WRAPPER
}

void JavascriptWrapper::saveFileFromUrl(QString url, QString saveFileWindowCaption, QString fileName, bool openAfterSave) {
BEGIN_SLOT_WRAPPER
    LOG << "Save file from url";
    const QString beginPath = makePath(walletPath, fileName);
    emit utilsManager.saveFileFromUrl(url, saveFileWindowCaption, beginPath, openAfterSave, utils::Utils::SaveFileFromUrlCallback([]{}, [](const TypedException &/*e*/) {}, std::bind(&JavascriptWrapper::callbackCall, this, _1)));
END_SLOT_WRAPPER
}

void JavascriptWrapper::chooseFileAndLoad(QString requestId, QString openFileWindowCaption, QString fileName) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "loadFileResultJs";

    LOG << "change file and load " << requestId;

    emit utilsManager.chooseFileAndLoad(openFileWindowCaption, makePath(walletPath, fileName), "", utils::Utils::ChooseFileAndLoadCallback([requestId, this, JS_NAME_RESULT](const QString &/*pathToFile*/, const std::string &result){
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<std::string>(result));
    }, [requestId, this, JS_NAME_RESULT](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<std::string>(""));
    }, std::bind(&JavascriptWrapper::callbackCall, this, _1)));
END_SLOT_WRAPPER
}

void JavascriptWrapper::qrEncode(QString requestId, QString textHex) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "qrEncodeResultJs";

    LOG << "qr encode";

    emit utilsManager.qrEncode(textHex, utils::Utils::QrEncodeCallback([requestId, this, JS_NAME_RESULT](const QString &result){
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
    }, [requestId, this, JS_NAME_RESULT](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>(""));
    }, std::bind(&JavascriptWrapper::callbackCall, this, _1)));
END_SLOT_WRAPPER
}

void JavascriptWrapper::qrDecode(QString requestId, QString pngBase64) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "qrDecodeResultJs";

    LOG << "qr decode";

    emit utilsManager.qrDecode(pngBase64, utils::Utils::QrEncodeCallback([requestId, this, JS_NAME_RESULT](const QString &result){
        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<QString>(result));
    }, [requestId, this, JS_NAME_RESULT](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, Opt<QString>(requestId), Opt<QString>(""));
    }, std::bind(&JavascriptWrapper::callbackCall, this, _1)));
END_SLOT_WRAPPER
}

void JavascriptWrapper::getAppInfo(const QString requestId) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getAppInfoResultJs";

    LOG << "get app info";

    const std::string versionString = VERSION_STRING;
    const std::string gitCommit = GIT_CURRENT_SHA1;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(requestId), Opt<bool>(isProductionSetup), Opt<std::string>(versionString), Opt<std::string>(gitCommit));
END_SLOT_WRAPPER
}

void JavascriptWrapper::onDirChanged(const QString &dir) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "directoryChangedResultJs";
    const QDir d(dir);
    for (const FolderWalletInfo &folderInfo: folderWalletsInfos) {
        if (folderInfo.walletPath == d) {
            LOG << "folder changed " << folderInfo.nameWallet << " " << d.absolutePath();
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(d.absolutePath()), Opt<QString>(folderInfo.nameWallet));
        }
    }
END_SLOT_WRAPPER
}

void JavascriptWrapper::metaOnline() {
BEGIN_SLOT_WRAPPER
    LOG << "Metaonline request";
    emit wssClient.sendMessage("{\"app\":\"MetaOnline\"}");
END_SLOT_WRAPPER
}

void JavascriptWrapper::clearNsLookup() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "clearNsLookupResultJs";
    LOG << "Clear ns lookup";
    nsLookup.resetFile();
    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>("Ok"));
END_SLOT_WRAPPER
}

void JavascriptWrapper::sendMessageToWss(QString message) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "sendMessageToWssResultJs";
    LOG << "Send message to wss: " << message;
    emit wssClient.sendMessage(message);
    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>("Ok"));
END_SLOT_WRAPPER
}

void JavascriptWrapper::setIsForgingActive(bool isActive) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "setIsForgingActiveResultJs";
    LOG << "Set is forging active: " << isActive;
    const TypedException exception = apiVrapper2([&, this](){
        QSettings settings(getRuntimeSettingsPath(), QSettings::IniFormat);
        settings.setValue("forging/enabled", isActive);
        settings.sync();

        sendAppInfoToWss(userName, true);
    });
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>("Ok"));
END_SLOT_WRAPPER
}

void JavascriptWrapper::getIsForgingActive() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getIsForgingActiveResultJs";
    QSettings settings(getRuntimeSettingsPath(), QSettings::IniFormat);
    const bool isForgingActive = settings.value("forging/enabled", true).toBool();
    LOG << "Get is forging active: " << isForgingActive;
    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<bool>(isForgingActive));
END_SLOT_WRAPPER
}

static QJsonDocument makeNetworkStatusResponse(const std::vector<NetwrokTesting::TestResult> &networkTestsResults, const std::vector<NodeTypeStatus> &nodeStatuses, const DnsErrorDetails &dnsError) {
    QJsonObject result;
    QJsonArray networkTestsJson;
    for (const NetwrokTesting::TestResult &r: networkTestsResults) {
        QJsonObject rJson;
        rJson.insert("node", r.host);
        rJson.insert("isTimeout", r.isTimeout);
        rJson.insert("timeMs", r.timeMs);
        networkTestsJson.push_back(rJson);
    }
    result.insert("networkTests", networkTestsJson);

    QJsonObject dnsErrorJson;
    if (!dnsError.isEmpty()) {
        dnsErrorJson.insert("node", dnsError.dnsName);
    }
    result.insert("dnsErrors", dnsErrorJson);

    QJsonArray nodesStatusesJson;
    for (const NodeTypeStatus &st: nodeStatuses) {
        QJsonObject stJson;
        stJson.insert("node", st.node);
        stJson.insert("countWorked", (int)st.countWorked);
        stJson.insert("countAll", (int)st.countAll);
        stJson.insert("bestTime", (int)st.bestResult);
        nodesStatusesJson.push_back(stJson);
    }
    result.insert("dnsStats", nodesStatusesJson);
    return QJsonDocument(result);
}

void JavascriptWrapper::getNetworkStatus() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getNetworkStatusResultJs";
    networkTesting.getTestResults(NetwrokTesting::GetTestResultsCallback([this, JS_NAME_RESULT](const std::vector<NetwrokTesting::TestResult> &networkTestsResults) {
        nsLookup.getStatus(NsLookup::GetStatusCallback([this, JS_NAME_RESULT, networkTestsResults](const std::vector<NodeTypeStatus> &nodeStatuses, const DnsErrorDetails &dnsError) {
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QJsonDocument>(makeNetworkStatusResponse(networkTestsResults, nodeStatuses, dnsError)));
        }, [](const TypedException &e) {
            LOG << "Error: " << e.description;
        }, std::bind(&JavascriptWrapper::callbackCall, this, _1)));
    }, [](const TypedException &e) {
        LOG << "Error: " << e.description;
    }, std::bind(&JavascriptWrapper::callbackCall, this, _1)));
END_SLOT_WRAPPER
}

void JavascriptWrapper::onWssMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
    CHECK(document.isObject(), "Message not is object");
    const QJsonObject root = document.object();
    if (!root.contains("app") || !root.value("app").isString()) {
        return;
    }
    const QString appType = root.value("app").toString();

    if (appType == QStringLiteral("MetaOnline")) {
        const QString JS_NAME_RESULT = "onlineResultJs";
        Opt<QJsonDocument> result;
        const TypedException exception = apiVrapper2([&](){
            CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
            const QJsonObject data = root.value("data").toObject();
            LOG << "Meta online response: " << message;
            result = QJsonDocument(data);
        });

        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    } else if (appType == QStringLiteral("InEvent")) {
        LOG << "EVENT: " << message;
        const QString event = root.value("event").toString();
        if (event == QStringLiteral("showExchangePopUp")) {
            const QString user = root.value("user").toString();
            const QString type = root.value("type").toString();
            if (user == userName) {
                const QString JS_NAME_RESULT = "showExchangePopUpJs";
                makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(type));
            }
        }
    }
END_SLOT_WRAPPER
}

void JavascriptWrapper::getAppModules(const QString requestId) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getAppModulesResultJs";

    LOG << "app modules";

    Opt<QJsonDocument> result;
    const TypedException exception = apiVrapper2([&](){
        const std::vector<std::pair<std::string, StatusModule>> modules = getStatusModules();
        QJsonArray allJson;
        for (const auto &module: modules) {
            QJsonObject moduleJson;
            moduleJson.insert("module", QString::fromStdString(module.first));
            const StatusModule &state = module.second;
            std::string stateStr;
            if (state == StatusModule::wait) {
                stateStr = "wait";
            } else if (state == StatusModule::notFound) {
                stateStr = "not_found";
            } else if (state == StatusModule::found) {
                stateStr = "found";
            } else {
                throwErr("Incorrect status module");
            }
            moduleJson.insert("state", QString::fromStdString(stateStr));
            allJson.append(moduleJson);
        }
        result = QJsonDocument(allJson);

        LOG << "app modules ok " << QString(result.get().toJson(QJsonDocument::Compact));
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::runJs(const QString &script) {
    emit jsRunSig(script);
}
