#include "JavascriptWrapper.h"

#include <map>

#include <QApplication>

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>

#include "Wallet.h"
#include "WalletRsa.h"
#include "EthWallet.h"
#include "BtcWallet.h"

#include "NsLookup.h"
#include "WebSocketClient.h"

#include "unzip.h"
#include "check.h"
#include "StopApplication.h"
#include "duration.h"
#include "Log.h"
#include "utils.h"
#include "TypedException.h"
#include "SlotWrapper.h"
#include "platform.h"
#include "Paths.h"
#include "makeJsFunc.h"
#include "qrcoder.h"

#include "machine_uid.h"

#include "transactions/Transactions.h"
#include "auth/Auth.h"

const static QString WALLET_PREV_PATH = ".metahash_wallets/";
const static QString WALLET_PATH_ETH = "eth/";
const static QString WALLET_PATH_BTC = "btc/";
const static QString WALLET_PATH_TMH_OLD = "mth/";
const static QString WALLET_PATH_TMH = "tmh/";

const QString JavascriptWrapper::defaultUsername = "_unregistered";

static QString makeCommandLineMessageForWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText) {
    QJsonObject allJson;
    allJson.insert("app", "MetaSearch");
    QJsonObject data;
    data.insert("machine_uid", hardwareId);
    data.insert("user_id", userId);
    data.insert("focus_release_count", (int)focusCount);
    data.insert("text", QString(line.toUtf8().toHex()));
    data.insert("is_enter_pressed", isEnter);
    data.insert("is_user_text", isUserText);
    allJson.insert("data", data);
    QJsonDocument json(allJson);

    return json.toJson(QJsonDocument::Compact);
}

static QString makeMessageApplicationForWss(const QString &hardwareId, const QString &userId, const QString &applicationVersion, const QString &interfaceVersion, const std::vector<QString> &keysTmh, const std::vector<QString> &keysMth) {
    QJsonObject allJson;
    allJson.insert("app", "MetaGate");
    QJsonObject data;
    data.insert("machine_uid", hardwareId);
    data.insert("user_id", userId);
    data.insert("application_ver", applicationVersion);
    data.insert("interface_ver", interfaceVersion);

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

JavascriptWrapper::JavascriptWrapper(WebSocketClient &wssClient, NsLookup &nsLookup, transactions::Transactions &transactionsManager, auth::Auth &authManager, const QString &applicationVersion, QObject */*parent*/)
    : walletDefaultPath(getWalletPath())
    , wssClient(wssClient)
    , nsLookup(nsLookup)
    , transactionsManager(transactionsManager)
    , applicationVersion(applicationVersion)
{
    hardwareId = QString::fromStdString(::getMachineUid());

    lastHtmls = Uploader::getLastHtmlVersion();

    LOG << "Wallets default path " << walletDefaultPath;

    CHECK(connect(&client, &SimpleClient::callbackCall, this, &JavascriptWrapper::onCallbackCall), "not connect callbackCall");
    CHECK(connect(this, &JavascriptWrapper::callbackCall, this, &JavascriptWrapper::onCallbackCall), "not connect callbackCall");

    CHECK(connect(&fileSystemWatcher, SIGNAL(directoryChanged(const QString&)), this, SLOT(onDirChanged(const QString&))), "not connect fileSystemWatcher");

    CHECK(connect(&wssClient, &WebSocketClient::messageReceived, this, &JavascriptWrapper::onWssMessageReceived), "not connect wssClient");

    CHECK(connect(this, &JavascriptWrapper::sendCommandLineMessageToWssSig, this, &JavascriptWrapper::onSendCommandLineMessageToWss), "not connect onSendCommandLineMessageToWss");

    CHECK(connect(&authManager, &auth::Auth::logined, this, &JavascriptWrapper::onLogined), "not connect onLogined");

    qRegisterMetaType<TypedException>("TypedException");
    qRegisterMetaType<ReturnCallback>("ReturnCallback");

    transactionsManager.setJavascriptWrapper(*this);

    emit authManager.reEmit();

    sendAppInfoToWss("", true);
}

void JavascriptWrapper::onCallbackCall(ReturnCallback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void JavascriptWrapper::setWidget(QWidget *widget) {
    widget_ = widget;
}

void JavascriptWrapper::onLogined(bool isInit, const QString login) {
BEGIN_SLOT_WRAPPER
    if (!login.isEmpty()) {
        setPathsImpl(makePath(walletDefaultPath, login), login);
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
        const std::vector<std::pair<QString, QString>> keys1 = Wallet::getAllWalletsInFolder(walletPathTmh);
        std::transform(keys1.begin(), keys1.end(), std::back_inserter(keysTmh), [](const auto &pair) {return pair.first;});
        std::vector<QString> keysMth;
        const std::vector<std::pair<QString, QString>> keys2 = Wallet::getAllWalletsInFolder(walletPathMth);
        std::transform(keys2.begin(), keys2.end(), std::back_inserter(keysMth), [](const auto &pair) {return pair.first;});

        const QString message = makeMessageApplicationForWss(hardwareId, newUserName, applicationVersion, lastHtmls.lastVersion, keysTmh, keysMth);
        emit wssClient.sendMessage(message);
        emit wssClient.setHelloString(message, "jsWrapper");
        sendedUserName = newUserName;
    }
}

void JavascriptWrapper::onSendCommandLineMessageToWss(const QString &hardwareId, const QString &userId, size_t focusCount, const QString &line, bool isEnter, bool isUserText) {
BEGIN_SLOT_WRAPPER
    emit wssClient.sendMessage(makeCommandLineMessageForWss(hardwareId, userId, focusCount, line, isEnter, isUserText));
END_SLOT_WRAPPER
}

////////////////
/// METAHASH ///
////////////////

void JavascriptWrapper::createWalletMTHS(QString requestId, QString password, QString walletPath, QString jsNameResult) {
    LOG << "Create wallet " << requestId;

    Opt<QString> walletFullPath;
    Opt<std::string> publicKey;
    Opt<std::string> address;
    Opt<std::string> exampleMessage;
    Opt<std::string> signature;

    const TypedException exception = apiVrapper2([&, this](){
        exampleMessage = "Example message " + std::to_string(rand());

        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        std::string pKey;
        std::string addr;
        Wallet::createWallet(walletPath, password.toStdString(), pKey, addr);

        pKey.clear();
        Wallet wallet(walletPath, addr, password.toStdString());
        signature = wallet.sign(exampleMessage.get(), pKey);
        publicKey = pKey;
        address = addr;

        LOG << "Create wallet ok " << requestId << " " << addr;

        walletFullPath = wallet.getFullPath();

        sendAppInfoToWss(userName, true);
    });

    makeAndRunJsFuncParams(jsNameResult, walletFullPath.getWithoutCheck(), exception, Opt<QString>(requestId), publicKey, address, exampleMessage, signature);
}

void JavascriptWrapper::createWallet(QString requestId, QString password) {
BEGIN_SLOT_WRAPPER
    createWalletMTHS(requestId, password, walletPathTmh, "createWalletResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::createWalletMHC(QString requestId, QString password) {
BEGIN_SLOT_WRAPPER
    createWalletMTHS(requestId, password, walletPathMth, "createWalletMHCResultJs");
END_SLOT_WRAPPER
}

QString JavascriptWrapper::getAllWalletsJson() {
    return getAllMTHSWalletsJson(walletPathTmh, "tmh");
}

QString JavascriptWrapper::getAllMHCWalletsJson() {
    return getAllMTHSWalletsJson(walletPathMth, "mhc");
}

QString JavascriptWrapper::getAllWalletsAndPathsJson() {
    return getAllMTHSWalletsAndPathsJson(walletPathTmh, "tmh");
}

QString JavascriptWrapper::getAllMHCWalletsAndPathsJson() {
    return getAllMTHSWalletsAndPathsJson(walletPathMth, "mhc");
}

void JavascriptWrapper::signMessage(QString requestId, QString keyName, QString text, QString password) {
BEGIN_SLOT_WRAPPER
    signMessageMTHS(requestId, keyName, text, password, walletPathTmh, "signMessageResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex) {
BEGIN_SLOT_WRAPPER
    signMessageMTHS(requestId, keyName, password, toAddress, value, fee, nonce, dataHex, walletPathTmh, "signMessageV2ResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageMTHSV3(requestId, keyName, password, toAddress, value, fee, nonce, dataHex, paramsJson, walletPathTmh, "signMessageV3ResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageDelegateMTHS(requestId, keyName, password, toAddress, value, fee, nonce, valueDelegate, true, paramsJson, walletPathTmh, "signMessageDelegateResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageUnDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageDelegateMTHS(requestId, keyName, password, toAddress, value, fee, nonce, valueDelegate, false, paramsJson, walletPathTmh, "signMessageUnDelegateResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHCDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageDelegateMTHS(requestId, keyName, password, toAddress, value, fee, nonce, valueDelegate, true, paramsJson, walletPathMth, "signMessageDelegateMhcResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHCUnDelegate(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageDelegateMTHS(requestId, keyName, password, toAddress, value, fee, nonce, valueDelegate, false, paramsJson, walletPathMth, "signMessageUnDelegateMhcResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHC(QString requestId, QString keyName, QString text, QString password) {
BEGIN_SLOT_WRAPPER
    signMessageMTHS(requestId, keyName, text, password, walletPathMth, "signMessageMHCResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHCV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex) {
BEGIN_SLOT_WRAPPER
    signMessageMTHS(requestId, keyName, password, toAddress, value, fee, nonce, dataHex, walletPathMth, "signMessageMHCV2ResultJs");
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMHCV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    signMessageMTHSV3(requestId, keyName, password, toAddress, value, fee, nonce, dataHex, paramsJson, walletPathMth, "signMessageMHCV3ResultJs");
END_SLOT_WRAPPER
}

static QString makeJsonWallets(const std::vector<std::pair<QString, QString>> &wallets) {
    QJsonArray jsonArray;
    for (const auto &r: wallets) {
        jsonArray.push_back(r.first);
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

QString JavascriptWrapper::getAllMTHSWalletsAndPathsJson(QString walletPath, QString name) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPath);
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

QString JavascriptWrapper::getAllMTHSWalletsJson(QString walletPath, QString name) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPath);
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

void JavascriptWrapper::checkAddress(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    LOG << "Check address " << address;
    const QString JS_NAME_RESULT = "checkAddressResultJs";
    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        try {
            Wallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            result = "not valid";
            return;
        } catch (...) {
            throw;
        }

        result = "ok";
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString text, QString password, QString walletPath, QString jsNameResult) {
    LOG << "Sign message " << requestId << " " << keyName << " " << text;

    const std::string textStr = text.toStdString();
    Opt<std::string> signature;
    Opt<std::string> publicKey;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string pubKey;
        signature = wallet.sign(textStr, pubKey);
        publicKey = pubKey;
    });

    makeAndRunJsFuncParams(jsNameResult, exception, Opt<QString>(requestId), signature, publicKey);
}

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString walletPath, QString jsNameResult) {
    LOG << "Sign message " << requestId << " " << keyName << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << dataHex;

    if (fee.isEmpty()) {
        fee = "0";
    }

    Opt<std::string> publicKey2;
    Opt<std::string> tx2;
    Opt<std::string> signature2;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        std::string tx;
        std::string signature;
        bool tmp;
        const uint64_t valueInt = value.toULongLong(&tmp, 10);
        CHECK(tmp, "Value not valid");
        const uint64_t feeInt = fee.toULongLong(&tmp, 10);
        CHECK(tmp, "Fee not valid");
        const uint64_t nonceInt = nonce.toULongLong(&tmp, 10);
        CHECK(tmp, "Nonce not valid");
        wallet.sign(toAddress.toStdString(), valueInt, feeInt, nonceInt, dataHex.toStdString(), tx, signature, publicKey);
        publicKey2 = publicKey;
        tx2 = tx;
        signature2 = signature;
    });

    makeAndRunJsFuncParams(jsNameResult, exception, Opt<QString>(requestId), signature2, publicKey2, tx2);
}

static transactions::Transactions::SendParameters parseSendParams(const QString &paramsJson) {
    transactions::Transactions::SendParameters result;
    const QJsonDocument doc = QJsonDocument::fromJson(paramsJson.toUtf8());
    CHECK_TYPED(doc.isObject(), TypeErrors::INCORRECT_USER_DATA, "params json incorrect");
    const QJsonObject docParams = doc.object();
    CHECK_TYPED(docParams.contains("countServersSend") && docParams.value("countServersSend").isDouble(), TypeErrors::INCORRECT_USER_DATA, "countServersSend not found in params");
    result.countServersSend = docParams.value("countServersSend").toInt();
    CHECK_TYPED(docParams.contains("countServersGet") && docParams.value("countServersGet").isDouble(), TypeErrors::INCORRECT_USER_DATA, "countServersGet not found in params");
    result.countServersGet = docParams.value("countServersSend").toInt();
    CHECK_TYPED(docParams.contains("typeSend") && docParams.value("typeSend").isString(), TypeErrors::INCORRECT_USER_DATA, "typeSend not found in params");
    result.typeSend = docParams.value("typeSend").toString();
    CHECK_TYPED(docParams.contains("typeGet") && docParams.value("typeGet").isString(), TypeErrors::INCORRECT_USER_DATA, "typeGet not found in params");
    result.typeGet = docParams.value("typeGet").toString();
    CHECK_TYPED(docParams.contains("timeout_sec") && docParams.value("timeout_sec").isDouble(), TypeErrors::INCORRECT_USER_DATA, "timeout_sec not found in params");
    result.timeout = seconds(docParams.value("timeout_sec").toInt());
    return result;
}

void JavascriptWrapper::createV8AddressImpl(QString requestId, const QString jsNameResult, QString address, int nonce) {
    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        result = QString::fromStdString(Wallet::createV8Address(address.toStdString(), nonce));
    });
    makeAndRunJsFuncParams(jsNameResult, exception, Opt<QString>(requestId), result);
}

void JavascriptWrapper::signMessageMTHSWithTxManager(const QString &requestId, const QString &walletPath, const QString jsNameResult, const QString &nonce, const QString &keyName, const QString &password, const QString &paramsJson, const std::function<void(size_t nonce)> &signTransaction) {
    const TypedException exception = apiVrapper2([&, this]() {
        const transactions::Transactions::SendParameters sendParams = parseSendParams(paramsJson);

        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        const bool isNonce = !nonce.isEmpty();
        if (!isNonce) {
            Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
            emit transactionsManager.getNonce(requestId, QString::fromStdString(wallet.getAddress()), sendParams, [this, jsNameResult, requestId, signTransaction, keyName](size_t nonce, const QString &server, const TypedException &exception) {
                Opt<QString> result(QString("Not ok"));
                const TypedException &exception2 = apiVrapper2([&] {
                    CHECK_TYPED(!exception.isSet(), exception.numError, exception.description);
                    LOG << "Nonce getted " << keyName << " " << nonce;
                    signTransaction(nonce);
                    result = "Ok";
                });
                makeAndRunJsFuncParams(jsNameResult, exception2, Opt<QString>(requestId), result);
            });
        } else {
            bool isParseNonce = false;
            const size_t nonceInt = nonce.toULongLong(&isParseNonce);
            CHECK_TYPED(isParseNonce, TypeErrors::INCORRECT_USER_DATA, "Nonce incorrect " + nonce.toStdString());
            signTransaction(nonceInt);
        }
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(jsNameResult, exception, Opt<QString>(requestId), Opt<QString>("Not ok"));
    }
}

void JavascriptWrapper::signMessageMTHSV3(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString dataHex, QString paramsJson, QString walletPath, QString jsNameResult) {
    LOG << "Sign messagev3 " << requestId << " " << keyName << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << dataHex;

    const transactions::Transactions::SendParameters sendParams = parseSendParams(paramsJson);

    if (fee.isEmpty()) {
        fee = "0";
    }

    const auto signTransaction = [this, requestId, walletPath, keyName, password, toAddress, value, fee, dataHex, sendParams](size_t nonce) {
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        std::string tx;
        std::string signature;
        bool tmp;
        const uint64_t valueInt = value.toULongLong(&tmp, 10);
        CHECK(tmp, "Value not valid");
        const uint64_t feeInt = fee.toULongLong(&tmp, 10);
        CHECK(tmp, "Fee not valid");
        wallet.sign(toAddress.toStdString(), valueInt, feeInt, nonce, dataHex.toStdString(), tx, signature, publicKey);

        emit transactionsManager.sendTransaction(requestId, toAddress, value, nonce, dataHex, fee, QString::fromStdString(publicKey), QString::fromStdString(signature), sendParams);
    };
    signMessageMTHSWithTxManager(requestId, walletPath, jsNameResult, nonce, keyName, password, paramsJson, signTransaction);
}

void JavascriptWrapper::signMessageDelegateMTHS(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString valueDelegate, bool isDelegate, QString paramsJson, QString walletPath, QString jsNameResult) {
    LOG << "Sign message delegate " << requestId << " " << keyName << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << isDelegate << " " << valueDelegate;

    const transactions::Transactions::SendParameters sendParams = parseSendParams(paramsJson);

    if (fee.isEmpty()) {
        fee = "0";
    }

    const auto signTransaction = [this, requestId, walletPath, keyName, password, toAddress, value, fee, valueDelegate, isDelegate, sendParams](size_t nonce) {
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());

        bool isValid;
        const uint64_t delegValue = valueDelegate.toULongLong(&isValid);
        CHECK(isValid, "delegate value not valid");
        const std::string dataHex = Wallet::genDataDelegateHex(isDelegate, delegValue);

        std::string publicKey;
        std::string tx;
        std::string signature;
        bool tmp;
        const uint64_t valueInt = value.toULongLong(&tmp, 10);
        CHECK(tmp, "Value not valid");
        const uint64_t feeInt = fee.toULongLong(&tmp, 10);
        CHECK(tmp, "Fee not valid");
        wallet.sign(toAddress.toStdString(), valueInt, feeInt, nonce, dataHex, tx, signature, publicKey, false);

        emit transactionsManager.sendTransaction(requestId, toAddress, value, nonce, QString::fromStdString(dataHex), fee, QString::fromStdString(publicKey), QString::fromStdString(signature), sendParams);
    };
    signMessageMTHSWithTxManager(requestId, walletPath, jsNameResult, nonce, keyName, password, paramsJson, signTransaction);
}

void JavascriptWrapper::getOnePrivateKeyMTHS(QString requestId, QString keyName, bool isCompact, QString walletPath, QString jsNameResult, bool isTmh) {
    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = Wallet::getPrivateKey(walletPath, keyName.toStdString(), isCompact, isTmh);

        QString res = QString::fromStdString(privKey);
        res.replace("\n", "\\n");
        res.replace("\r", "");
        result = res;

        LOG << "Getted private key " << keyName;
    });

    makeAndRunJsFuncParams(jsNameResult, exception, Opt<QString>(requestId), result);
}

void JavascriptWrapper::getOnePrivateKey(QString requestId, QString keyName, bool isCompact) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getOnePrivateKeyResultJs";
    getOnePrivateKeyMTHS(requestId, keyName, isCompact, walletPathTmh, JS_NAME_RESULT, true);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getOnePrivateKeyMHC(QString requestId, QString keyName, bool isCompact) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getOnePrivateKeyMHCResultJs";
    getOnePrivateKeyMTHS(requestId, keyName, isCompact, walletPathMth, JS_NAME_RESULT, false);
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyMTHS(QString requestId, QString privateKey, QString password, QString walletPath, QString jsNameResult) {
    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key";

        Wallet::savePrivateKey(walletPath, privateKey.toStdString(), password.toStdString());
        result = "ok";
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        result = "Not ok";
    }
    makeAndRunJsFuncParams(jsNameResult, exception, Opt<QString>(requestId), result);
}

void JavascriptWrapper::savePrivateKey(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";
    savePrivateKeyMTHS(requestId, privateKey, password, walletPathTmh, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyMHC(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";
    savePrivateKeyMTHS(requestId, privateKey, password, walletPathMth, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::saveRawPrivKeyMTHS(QString requestId, QString rawPrivKey, QString password, QString walletPath, QString jsNameResult) {
    std::string pubkey;
    Opt<std::string> address;
    const TypedException exception = apiVrapper2([&]() {
        std::string addr;
        Wallet::createWalletFromRaw(walletPath, rawPrivKey.toStdString(), password.toStdString(), pubkey, addr);
        address = addr;
    });
    makeAndRunJsFuncParams(jsNameResult, exception, Opt<QString>(requestId), address);
}

void JavascriptWrapper::saveRawPrivKey(QString requestId, QString rawPrivKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "saveRawPrivkeyResultJs";
    saveRawPrivKeyMTHS(requestId, rawPrivKey, password, walletPathTmh, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::saveRawPrivKeyMHC(QString requestId, QString rawPrivKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "saveRawPrivkeyMHCResultJs";
    saveRawPrivKeyMTHS(requestId, rawPrivKey, password, walletPathMth, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getRawPrivKeyMTHS(QString requestId, QString address, QString password, QString walletPath, QString jsNameResult) {
    Opt<std::string> result;
    const TypedException exception = apiVrapper2([&]() {
        Wallet wallet(walletPath, address.toStdString(), password.toStdString());
        result = wallet.getNotProtectedKeyHex();
    });
    makeAndRunJsFuncParams(jsNameResult, exception, Opt<QString>(requestId), result);
}

void JavascriptWrapper::getRawPrivKey(QString requestId, QString address, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getRawPrivkeyResultJs";
    getRawPrivKeyMTHS(requestId, address, password, walletPathTmh, JS_NAME_RESULT);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getRawPrivKeyMHC(QString requestId, QString address, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getRawPrivkeyMHCResultJs";
    getRawPrivKeyMTHS(requestId, address, password, walletPathMth, JS_NAME_RESULT);
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

void JavascriptWrapper::createRsaKey(QString requestId, QString address, QString password) {
BEGIN_SLOT_WRAPPER
    LOG << "Create rsa key " << address;

    const QString JS_NAME_RESULT = "createRsaKeyResultJs";
    Opt<std::string> publicKey;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        WalletRsa::createRsaKey(walletPathMth, address.toStdString(), password.toStdString());
        WalletRsa wallet(walletPathMth, address.toStdString());
        publicKey = wallet.getPublikKey();
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), publicKey);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getRsaPublicKey(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    LOG << "Get rsa key " << address;

    const QString JS_NAME_RESULT = "getRsaPublicKeyResultJs";
    Opt<std::string> publicKey;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        WalletRsa wallet(walletPathMth, address.toStdString());
        publicKey = wallet.getPublikKey();
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), publicKey);
END_SLOT_WRAPPER
}

void JavascriptWrapper::encryptMessage(QString requestId, QString publicKey, QString message) {
BEGIN_SLOT_WRAPPER
    LOG << "encrypt message";

    const QString JS_NAME_RESULT = "encryptMessageResultJs";
    Opt<std::string> answer;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
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
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        WalletRsa wallet(walletPathMth, addr.toStdString());
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
    Opt<std::string> address;
    QString fullPath;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        address = EthWallet::genPrivateKey(walletPathEth, password.toStdString());

        fullPath = EthWallet::getFullPath(walletPathEth, address.get());
        LOG << "Create eth wallet ok " << requestId << " " << address.get();
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, fullPath, exception, Opt<QString>(requestId), address);
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "signMessageEthResultJs";

    LOG << "Sign message eth " << address << " " << nonce << " " << gasPrice << " " << gasLimit << " " << to << " " << value << " " << data;

    Opt<std::string> result;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        EthWallet wallet(walletPathEth, address.toStdString(), password.toStdString());
        result = wallet.SignTransaction(
            nonce.toStdString(),
            gasPrice.toStdString(),
            gasLimit.toStdString(),
            to.toStdString(),
            value.toStdString(),
            data.toStdString()
        );
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::checkAddressEth(QString requestId, QString address) {
BEGIN_SLOT_WRAPPER
    LOG << "Check address eth " << address;
    const QString JS_NAME_RESULT = "checkAddressEthResultJs";
    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        try {
            EthWallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            result = "not valid";
            return;
        } catch (...) {
            throw;
        }

        result = "ok";
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
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
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = EthWallet::getAllWalletsInFolder(walletPathEth);
        const QString jsonStr = makeJsonWallets(result);
        LOG << PeriodicLog::make("w_eth") << "get eth wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString JavascriptWrapper::getAllEthWalletsAndPathsJson() {
    try {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = EthWallet::getAllWalletsInFolder(walletPathEth);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << PeriodicLog::make("w2_eth") << "get eth wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

void JavascriptWrapper::getOnePrivateKeyEth(QString requestId, QString keyName) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getOnePrivateKeyEthResultJs";

    LOG << "get one private key eth " << keyName;

    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = EthWallet::getOneKey(walletPathEth, keyName.toStdString());

        QString r = QString::fromStdString(privKey);
        r.replace("\"", "\\\"");
        r.replace("\n", "\\n");
        result = r;
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyEth(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";

    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key eth";

        EthWallet::savePrivateKey(walletPathEth, privateKey.toStdString(), password.toStdString());
        result = "ok";
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        result = "Not ok";
    }
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

///////////////
/// BITCOIN ///
///////////////

void JavascriptWrapper::createWalletBtcPswd(QString requestId, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "createWalletBtcResultJs";

    LOG << "Create wallet btc " << requestId;

    Opt<std::string> address;
    QString fullPath;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        address = BtcWallet::genPrivateKey(walletPathBtc, password).first;

        fullPath = BtcWallet::getFullPath(walletPathBtc, address.get());
        LOG << "Create btc wallet ok " << requestId << " " << address.get();
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, fullPath, exception, Opt<QString>(requestId), address);
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
    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        try {
            BtcWallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            result = "not valid";
            return;
        } catch (...) {
            throw;
        }

        result = "ok";
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

// deprecated
void JavascriptWrapper::signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "signMessageBtcResultJs";

    LOG << "Sign message btc " << address << " " << toAddress << " " << value << " " << estimateComissionInSatoshi << " " << fees;

    Opt<std::string> result;
    const TypedException exception = apiVrapper2([&, this]() {
        std::vector<BtcInput> btcInputs;

        const QJsonDocument document = QJsonDocument::fromJson(jsonInputs.toUtf8());
        CHECK(document.isArray(), "jsonInputs not array");
        const QJsonArray root = document.array();
        for (const auto &jsonObj2: root) {
            const QJsonObject jsonObj = jsonObj2.toObject();
            BtcInput input;
            CHECK(jsonObj.contains("value") && jsonObj.value("value").isString(), "value field not found");
            bool isValid;
            input.outBalance = jsonObj.value("value").toString().toULongLong(&isValid);
            CHECK(isValid, "Out balance not valid");
            CHECK(jsonObj.contains("scriptPubKey") && jsonObj.value("scriptPubKey").isString(), "scriptPubKey field not found");
            input.scriptPubkey = jsonObj.value("scriptPubKey").toString().toStdString();
            CHECK(jsonObj.contains("tx_index") && jsonObj.value("tx_index").isDouble(), "tx_index field not found");
            input.spendoutnum = jsonObj.value("tx_index").toInt();
            CHECK(jsonObj.contains("tx_hash") && jsonObj.value("tx_hash").isString(), "tx_hash field not found");
            input.spendtxid = jsonObj.value("tx_hash").toString().toStdString();
            btcInputs.emplace_back(input);
        }

        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        BtcWallet wallet(walletPathBtc, address.toStdString(), password);
        size_t estimateComissionInSatoshiInt = 0;
        if (!estimateComissionInSatoshi.isEmpty()) {
            CHECK(isDecimal(estimateComissionInSatoshi.toStdString()), "Not hex number value");
            estimateComissionInSatoshiInt = std::stoll(estimateComissionInSatoshi.toStdString());
        }
        const auto resultPair = wallet.buildTransaction(btcInputs, estimateComissionInSatoshiInt, value.toStdString(), fees.toStdString(), toAddress.toStdString());
        result = resultPair.first;
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::signMessageBtcPswdUsedUtxos(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees, QString jsonUsedUtxos) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "signMessageBtcUsedUtxosResultJs";

    LOG << "Sign message btc utxos " << address << " " << toAddress << " " << value << " " << estimateComissionInSatoshi << " " << fees;

    Opt<QJsonDocument> jsonUtxos;
    Opt<std::string> transactionHash;
    Opt<std::string> result;
    const TypedException exception = apiVrapper2([&, this]() {
        std::vector<BtcInput> btcInputs;

        const QJsonDocument document = QJsonDocument::fromJson(jsonInputs.toUtf8());
        CHECK(document.isArray(), "jsonInputs not array");
        const QJsonArray root = document.array();
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

        std::set<std::string> usedUtxos;
        const QJsonDocument documentUsed = QJsonDocument::fromJson(jsonUsedUtxos.toUtf8());
        CHECK(documentUsed.isArray(), "jsonInputs not array");
        const QJsonArray rootUsed = documentUsed.array();
        for (const auto &jsonUsedUtxo: rootUsed) {
            CHECK(jsonUsedUtxo.isString(), "value field not found");
            usedUtxos.insert(jsonUsedUtxo.toString().toStdString());
        }
        btcInputs = BtcWallet::reduceInputs(btcInputs, usedUtxos);
        LOG << "Used utxos: " << usedUtxos.size();

        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        BtcWallet wallet(walletPathBtc, address.toStdString(), password);
        size_t estimateComissionInSatoshiInt = 0;
        if (!estimateComissionInSatoshi.isEmpty()) {
            CHECK(isDecimal(estimateComissionInSatoshi.toStdString()), "Not hex number value");
            estimateComissionInSatoshiInt = std::stoll(estimateComissionInSatoshi.toStdString());
        }
        const auto resultPair = wallet.buildTransaction(btcInputs, estimateComissionInSatoshiInt, value.toStdString(), fees.toStdString(), toAddress.toStdString());
        result = resultPair.first;
        const std::set<std::string> &thisUsedTxs = resultPair.second;
        usedUtxos.insert(thisUsedTxs.begin(), thisUsedTxs.end());

        QJsonArray jsonArrayUtxos;
        for (const std::string &r: usedUtxos) {
            jsonArrayUtxos.push_back(QString::fromStdString(r));
        }
        jsonUtxos = QJsonDocument(jsonArrayUtxos);

        transactionHash = BtcWallet::calcHashNotWitness(result.get());
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result, jsonUtxos, transactionHash);
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
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = BtcWallet::getAllWalletsInFolder(walletPathBtc);
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
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = BtcWallet::getAllWalletsInFolder(walletPathBtc);
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

    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = BtcWallet::getOneKey(walletPathBtc, keyName.toStdString());

        QString r = QString::fromStdString(privKey);
        r.replace("\n", "\\n");
        result = r;
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::savePrivateKeyBtc(QString requestId, QString privateKey, QString password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";

    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key btc";

        BtcWallet::savePrivateKey(walletPathBtc, privateKey.toStdString(), password);
        result = "ok";
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        result = "Not ok";
    }
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
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
    const TypedException exception = apiVrapper2([&, this]() {
        updateAndRestart();
        result = "Ok";
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::qtOpenInBrowser(QString url) {
BEGIN_SLOT_WRAPPER
    LOG << "Open another url " << url;
    QDesktopServices::openUrl(QUrl(url));
END_SLOT_WRAPPER
}

void JavascriptWrapper::getWalletFolders() {
BEGIN_SLOT_WRAPPER
    LOG << "getWalletFolders ";
    const QString JS_NAME_RESULT = "walletFoldersJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(walletDefaultPath), Opt<QString>(walletPath), Opt<QString>(userName));
END_SLOT_WRAPPER
}

bool JavascriptWrapper::migrateKeysToPath(QString newPath) {
    LOG << "Migrate keys to path " << newPath;

    const QString prevPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_PREV_PATH);

    copyRecursively(makePath(prevPath, WALLET_PATH_ETH), makePath(newPath, WALLET_PATH_ETH), false);
    copyRecursively(makePath(prevPath, WALLET_PATH_BTC), makePath(newPath, WALLET_PATH_BTC), false);
    copyRecursively(makePath(prevPath, Wallet::WALLET_PATH_MTH), makePath(newPath, Wallet::WALLET_PATH_MTH), false);
    copyRecursively(makePath(prevPath, WALLET_PATH_TMH), makePath(newPath, WALLET_PATH_TMH), false);
    copyRecursively(prevPath, makePath(newPath, WALLET_PATH_TMH), false);

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

    auto setPathToWallet = [this](QString &curPath, const QString &suffix, const QString &name) {
        curPath = makePath(walletPath, suffix);
        createFolder(curPath);
        folderWalletsInfos.emplace_back(curPath, name);
        fileSystemWatcher.addPath(curPath);
    };

    setPathToWallet(walletPathEth, WALLET_PATH_ETH, "eth");
    setPathToWallet(walletPathBtc, WALLET_PATH_BTC, "btc");
    setPathToWallet(walletPathMth, Wallet::WALLET_PATH_MTH, "mhc");
    setPathToWallet(walletPathTmh, WALLET_PATH_TMH, "tmh");

    walletPathOldTmh = makePath(walletPath, WALLET_PATH_TMH_OLD);
    LOG << "Wallets path " << walletPath;

    QDir oldTmhPath(walletPathOldTmh);
    if (oldTmhPath.exists()) {
        copyRecursively(walletPathOldTmh, walletPathTmh, true);
        oldTmhPath.removeRecursively();
    }

    sendAppInfoToWss(newUserName, false);
}

void JavascriptWrapper::setPaths(QString newPatch, QString newUserName) {
BEGIN_SLOT_WRAPPER
    /*const QString JS_NAME_RESULT = "setPathsJs";
    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        setPathsImpl(newPatch, newUserName);
        result = "Ok";
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        result = "Not ok";
    }

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);*/
END_SLOT_WRAPPER
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

void JavascriptWrapper::setHasNativeToolbarVariable() {
BEGIN_SLOT_WRAPPER
    emit setHasNativeToolbarVariableSig();
END_SLOT_WRAPPER
}

void JavascriptWrapper::lineEditReturnPressed(QString text) {
BEGIN_SLOT_WRAPPER
    emit lineEditReturnPressedSig(text);
END_SLOT_WRAPPER
}

void JavascriptWrapper::setCommandLineText(const QString &/*text*/) {
BEGIN_SLOT_WRAPPER
    //emit setCommandLineTextSig(text);
END_SLOT_WRAPPER
}

void JavascriptWrapper::openFolderInStandartExplored(const QString &folder) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void JavascriptWrapper::openWalletPathInStandartExplorer() {
BEGIN_SLOT_WRAPPER
    openFolderInStandartExplored(walletPath);
END_SLOT_WRAPPER
}

void JavascriptWrapper::setPagesMapping(QString mapping) {
BEGIN_SLOT_WRAPPER
    //emit setMappingsSig(mapping);
END_SLOT_WRAPPER
}

void JavascriptWrapper::getIpsServers(QString requestId, QString type, int length, int count) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getIpsServersJs";

    LOG << "get ips servers " << requestId;

    Opt<QString> res;
    const TypedException exception = apiVrapper2([&, this]() {
        const std::vector<QString> result = nsLookup.getRandomWithoutHttp(type, length, count);

        QString resultStr = "[";
        bool isFirst = true;
        for (const QString &r: result) {
            if (!isFirst) {
                resultStr += ", ";
            }
            isFirst = false;
            resultStr += "\\\"" + r + "\\\"";
        }
        resultStr += "]";

        res = resultStr;
    });

    LOG << "get ips servers ok " << requestId << " " << res.get();

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), res);
END_SLOT_WRAPPER
}

void JavascriptWrapper::setUserName(const QString &userName) {
BEGIN_SLOT_WRAPPER
    // ignore
END_SLOT_WRAPPER
}

void JavascriptWrapper::saveFileFromUrl(QString url, QString saveFileWindowCaption, QString fileName, bool openAfterSave) {
BEGIN_SLOT_WRAPPER
    LOG << "Save file from url";
    const QString beginPath = makePath(walletPath, fileName);
    const QString file = QFileDialog::getSaveFileName(widget_, saveFileWindowCaption, beginPath);
    CHECK(!file.isNull() && !file.isEmpty(), "File not changed");

    client.sendMessageGet(url, [this, file, openAfterSave](const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Error load image: " + exception.description);
        writeToFileBinary(file, response, false);
        if (openAfterSave) {
            openFolderInStandartExplored(QFileInfo(file).dir().path());
        }
    });
END_SLOT_WRAPPER
}

void JavascriptWrapper::chooseFileAndLoad(QString requestId, QString openFileWindowCaption, QString fileName) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "loadFileResultJs";

    LOG << "change file and load " << requestId;

    Opt<std::string> base64Data;
    const TypedException exception = apiVrapper2([&, this]() {
        const QString beginPath = makePath(walletPath, fileName);
        const QString file = QFileDialog::getOpenFileName(widget_, openFileWindowCaption, beginPath);
        const std::string fileData = readFileBinary(file);
        base64Data = toBase64(fileData);
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), base64Data);
END_SLOT_WRAPPER
}

void JavascriptWrapper::printUrl(QString url, QString printWindowCaption, QString text) {
BEGIN_SLOT_WRAPPER
    LOG << "print url";
    client.sendMessageGet(url, [printWindowCaption, text](const std::string &response, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Error load image: " + exception.description);

        QImage image;
        image.loadFromData((const unsigned char*)response.data(), (int)response.size());

        QPrinter printer;

        QPrintDialog *dialog = new QPrintDialog(&printer);
        dialog->setWindowTitle(printWindowCaption);

        if (dialog->exec() != QDialog::Accepted) {
            return;
        }

        QPainter painter;
        painter.begin(&printer);

        const int printerWidth = printer.pageRect().width();
        const int printerHeight = printer.pageRect().height();
        const int imageWidth = image.size().width();
        const int imageHeight = image.size().height();
        const int paddingX = (printerWidth - imageWidth) / 2;
        const int paddingY = (printerHeight - imageHeight) / 2;

        painter.drawText(100, 100, 500, 500, Qt::AlignLeft|Qt::AlignTop, text);
        painter.drawImage(QRect(paddingX, paddingY, imageWidth, imageHeight), image);

        painter.end();
    });
END_SLOT_WRAPPER
}

void JavascriptWrapper::qrEncode(QString requestId, QString textHex) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "qrEncodeResultJs";

    LOG << "qr encode";

    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this](){
        CHECK_TYPED(!textHex.isEmpty(), TypeErrors::INCORRECT_USER_DATA, "text for encode empty");
        const QByteArray data = QByteArray::fromHex(textHex.toUtf8());
        const QByteArray res = QRCoder::encode(data);
        CHECK_TYPED(res.size() > 0, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect result");
        const QByteArray check = QRCoder::decode(res);
        CHECK_TYPED(check == data, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect check result");
        result = QString(res.toBase64());
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void JavascriptWrapper::qrDecode(QString requestId, QString pngBase64) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "qrDecodeResultJs";

    LOG << "qr decode";

    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this](){
        CHECK_TYPED(!pngBase64.isEmpty(), TypeErrors::INCORRECT_USER_DATA, "text for encode empty");
        const QByteArray data = QByteArray::fromBase64(pngBase64.toUtf8());
        const QByteArray res = QRCoder::decode(data);
        CHECK_TYPED(res.size() > 0, TypeErrors::QR_ENCODE_ERROR, "Incorrect encoded qr: incorrect result");
        result = QString(res.toHex());
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
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

void JavascriptWrapper::onWssMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8());
    CHECK(document.isObject(), "Message not is object");
    const QJsonObject root = document.object();
    CHECK(root.contains("app") && root.value("app").isString(), "app field not found");
    const std::string appType = root.value("app").toString().toStdString();

    if (appType == "MetaOnline") {
        const QString JS_NAME_RESULT = "onlineResultJs";
        Opt<QJsonDocument> result;
        const TypedException exception = apiVrapper2([&, this](){
            CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
            const QJsonObject data = root.value("data").toObject();
            LOG << "Meta online response";
            result = QJsonDocument(data);
        });

        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    }
END_SLOT_WRAPPER
}

void JavascriptWrapper::runJs(const QString &script) {
    emit jsRunSig(script);
}
