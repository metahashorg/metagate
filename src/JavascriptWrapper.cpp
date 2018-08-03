#include "JavascriptWrapper.h"

#include <map>

#include <QApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDesktopServices>
#include <QFileDialog>
#include <QMessageBox>
#include <QPrinter>
#include <QPrintDialog>
#include <QPainter>

#include "Wallet.h"
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

#include "machine_uid.h"

const static QString WALLET_PREV_PATH = ".metahash_wallets/";
const static QString WALLET_PATH_ETH = "eth/";
const static QString WALLET_PATH_BTC = "btc/";
const static QString WALLET_PATH_MTH = "mhc/";
const static QString WALLET_PATH_TMH_OLD = "mth/";
const static QString WALLET_PATH_TMH = "tmh/";

JavascriptWrapper::JavascriptWrapper(WebSocketClient &wssClient, NsLookup &nsLookup, QObject */*parent*/)
    : wssClient(wssClient)
    , nsLookup(nsLookup)
{
    hardwareId = QString::fromStdString(::getMachineUid());

    walletDefaultPath = getWalletPath();
    LOG << "Wallets default path " << walletDefaultPath;

    setPaths(walletDefaultPath, "");

    CHECK(connect(&client, SIGNAL(callbackCall(ReturnCallback)), this, SLOT(onCallbackCall(ReturnCallback))), "not connect callbackCall");

    CHECK(connect(&fileSystemWatcher, SIGNAL(directoryChanged(const QString&)), this, SLOT(onDirChanged(const QString&))), "not connect fileSystemWatcher");

    CHECK(connect(&wssClient, &WebSocketClient::messageReceived, this, &JavascriptWrapper::onWssMessageReceived), "not connect wssClient");
}

void JavascriptWrapper::onCallbackCall(ReturnCallback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void JavascriptWrapper::setWidget(QWidget *widget) {
    widget_ = widget;
}

template<class Function>
void JavascriptWrapper::apiVrapper(const QString &javascriptFunctionName, const QString &requestId, const Function &func) {
    // TODO когда будет if constexpr, объединить обе функции в одну
    using TypeReturn = typename std::result_of<decltype(func)()>::type;
    TypeReturn ret;
    try {
        const TypeReturn ret = func();
        const QString result = ret.invoke();
        runJs(result);
        return;
    } catch (const TypedException &e) {
        LOG << "Error " << std::to_string(e.numError) << ". " << e.description;
        ret = TypeReturn(javascriptFunctionName, e);
    } catch (const Exception &e) {
        LOG << "Error " << e;
        ret = TypeReturn(javascriptFunctionName, TypedException(TypeErrors::OTHER_ERROR, e));
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
        ret = TypeReturn(javascriptFunctionName, TypedException(TypeErrors::OTHER_ERROR, e.what()));
    } catch (...) {
        LOG << "Unknown error";
        ret = TypeReturn(javascriptFunctionName, TypedException(TypeErrors::OTHER_ERROR, "Unknown error"));
    }

    ret.setFirstArgument(requestId);
    const QString result = ret.invoke();
    runJs(result);
}

template<class Function>
void JavascriptWrapper::apiVrapper(const QString &javascriptFunctionName, const Function &func) {
    using TypeReturn = typename std::result_of<decltype(func)()>::type;
    TypeReturn ret;
    try {
        const TypeReturn ret = func();
        const QString result = ret.invoke();
        runJs(result);
        return;
    } catch (const TypedException &e) {
        LOG << "Error " << std::to_string(e.numError) << ". " << e.description;
        ret = TypeReturn(javascriptFunctionName, e);
    } catch (const Exception &e) {
        LOG << "Error " << e;
        ret = TypeReturn(javascriptFunctionName, TypedException(TypeErrors::OTHER_ERROR, e));
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
        ret = TypeReturn(javascriptFunctionName, TypedException(TypeErrors::OTHER_ERROR, e.what()));
    } catch (...) {
        LOG << "Unknown error";
        ret = TypeReturn(javascriptFunctionName, TypedException(TypeErrors::OTHER_ERROR, "Unknown error"));
    }

    const QString result = ret.invoke();
    runJs(result);
}

template<typename... Args>
JsFunc<true, Args...> JavascriptWrapper::makeJsFuncParams(const QString &function, const QString &lastArg, const TypedException &exception, Args&& ...args) {
    return makeJsFunc<true>(function, lastArg, exception, std::forward<Args>(args)...);
}

template<typename... Args>
JsFunc<false, Args...> JavascriptWrapper::makeJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    return makeJsFunc<false>(function, "", exception, std::forward<Args>(args)...);
}

////////////////
/// METAHASH ///
////////////////

void JavascriptWrapper::createWalletMTHS(QString requestId, QString password, QString walletPath, QString jsNameResult) {
    LOG << "Create wallet " << requestId;

    apiVrapper(jsNameResult, requestId, [&, this](){
        std::string publicKey;
        std::string addr;
        const std::string exampleMessage = "Example message " + std::to_string(rand());
        std::string signature;

        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::createWallet(walletPath, password.toStdString(), publicKey, addr);

        publicKey.clear();
        Wallet wallet(walletPath, addr, password.toStdString());
        signature = wallet.sign(exampleMessage, publicKey);

        return makeJsFuncParams(jsNameResult, wallet.getFullPath(), TypedException(), requestId, publicKey, addr, exampleMessage, signature);
    });

    LOG << "Create wallet ok " << requestId;
}

void JavascriptWrapper::createWallet(QString requestId, QString password) {
    createWalletMTHS(requestId, password, walletPathTmh, "createWalletResultJs");
}

void JavascriptWrapper::createWalletMHC(QString requestId, QString password) {
    createWalletMTHS(requestId, password, walletPathMth, "createWalletMHCResultJs");
}

QString JavascriptWrapper::getAllWalletsJson() {
    return getAllMTHSWalletsJson(walletPathTmh);
}

QString JavascriptWrapper::getAllMHCWalletsJson() {
    return getAllMTHSWalletsJson(walletPathMth);
}

QString JavascriptWrapper::getAllWalletsAndPathsJson() {
    return getAllMTHSWalletsAndPathsJson(walletPathTmh);
}

QString JavascriptWrapper::getAllMHCWalletsAndPathsJson() {
    return getAllMTHSWalletsAndPathsJson(walletPathMth);
}

void JavascriptWrapper::signMessage(QString requestId, QString keyName, QString text, QString password) {
    signMessageMTHS(requestId, keyName, text, password, walletPathTmh, "signMessageResultJs");
}

void JavascriptWrapper::signMessageV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString data) {
    signMessageMTHS(requestId, keyName, password, toAddress, value, fee, nonce, data, walletPathTmh, "signMessageV2ResultJs");
}

void JavascriptWrapper::signMessageMHC(QString requestId, QString keyName, QString text, QString password) {
    signMessageMTHS(requestId, keyName, text, password, walletPathMth, "signMessageMHCResultJs");
}

void JavascriptWrapper::signMessageMHCV2(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString data) {
    signMessageMTHS(requestId, keyName, password, toAddress, value, fee, nonce, data, walletPathMth, "signMessageMHCV2ResultJs");
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

QString JavascriptWrapper::getAllMTHSWalletsAndPathsJson(QString walletPath) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPath);
        const QString jsonStr = makeJsonWalletsAndPaths(result);
        LOG << "get mth wallets json " << jsonStr;
        return jsonStr;
    } catch (const Exception &e) {
        LOG << "Error: " + e;
        return "Error: " + QString::fromStdString(e);
    } catch (...) {
        LOG << "Unknown error";
        return "Unknown error";
    }
}

QString JavascriptWrapper::getAllMTHSWalletsJson(QString walletPath) {
    try {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = Wallet::getAllWalletsInFolder(walletPath);
        const QString jsonStr = makeJsonWallets(result);
        LOG << "get mth wallets json " << jsonStr;
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
    LOG << "Check address " << address;
    const QString JS_NAME_RESULT = "checkAddressResultJs";
    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        try {
            Wallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, QString("not valid"));
        } catch (...) {
            throw;
        }

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, QString("ok"));
    });
}

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString text, QString password, QString walletPath, QString jsNameResult) {
    LOG << "Sign message " << requestId << keyName << text;

    const std::string textStr = text.toStdString();

    apiVrapper(jsNameResult, requestId, [&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        const std::string signature = wallet.sign(textStr, publicKey);

        return makeJsFuncParams(jsNameResult, TypedException(), requestId, signature, publicKey);
    });
}

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString data, QString walletPath, QString jsNameResult) {
    LOG << "Sign message " << requestId << keyName << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << data;

    apiVrapper(jsNameResult, requestId, [&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        std::string tx;
        std::string signature;
        bool tmp;
        wallet.sign(toAddress.toStdString(), value.toULongLong(&tmp, 10), fee.toULongLong(&tmp, 10), nonce.toULongLong(&tmp, 10), data.toStdString(), tx, signature, publicKey);

        return makeJsFuncParams(jsNameResult, TypedException(), requestId, signature, publicKey, tx);
    });
}

void JavascriptWrapper::getOnePrivateKeyMTHS(QString requestId, QString keyName, bool isCompact, QString walletPath, QString jsNameResult, bool isTmh) {
    apiVrapper(jsNameResult, requestId, [&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = Wallet::getPrivateKey(walletPath, keyName.toStdString(), isCompact, isTmh);

        QString result = QString::fromStdString(privKey);
        result.replace("\n", "\\n");

        LOG << "Getted private key";

        return makeJsFuncParams(jsNameResult, TypedException(), requestId, result);
    });
}

void JavascriptWrapper::getOnePrivateKey(QString requestId, QString keyName, bool isCompact) {
    const QString JS_NAME_RESULT = "getOnePrivateKeyResultJs";
    getOnePrivateKeyMTHS(requestId, keyName, isCompact, walletPathTmh, JS_NAME_RESULT, true);
}

void JavascriptWrapper::getOnePrivateKeyMHC(QString requestId, QString keyName, bool isCompact) {
    const QString JS_NAME_RESULT = "getOnePrivateKeyMHCResultJs";
    getOnePrivateKeyMTHS(requestId, keyName, isCompact, walletPathMth, JS_NAME_RESULT, false);
}

void JavascriptWrapper::savePrivateKeyMTHS(QString requestId, QString privateKey, QString password, QString walletPath, QString jsNameResult) {
    apiVrapper(jsNameResult, requestId, [&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key";

        Wallet::savePrivateKey(walletPath, privateKey.toStdString(), password.toStdString());

        return makeJsFuncParams(jsNameResult, TypedException(), requestId, "ok");
    });
}

void JavascriptWrapper::savePrivateKey(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";
    savePrivateKeyMTHS(requestId, privateKey, password, walletPathTmh, JS_NAME_RESULT);
}

void JavascriptWrapper::savePrivateKeyMHC(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";
    savePrivateKeyMTHS(requestId, privateKey, password, walletPathMth, JS_NAME_RESULT);
}

void JavascriptWrapper::createRsaKey(QString requestId, QString address, QString password) {
    LOG << "Create rsa key";

    const QString JS_NAME_RESULT = "createRsaKeyResultJs";
    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::createRsaKey(walletPathMth, address.toStdString(), password.toStdString());
        const std::string publicKey = Wallet::getPublicRsaKey(walletPathMth, address.toStdString());

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, publicKey);
    });
}

void JavascriptWrapper::getRsaPublicKey(QString requestId, QString address) {
    LOG << "Get rsa key";

    const QString JS_NAME_RESULT = "getRsaPublicKeyResultJs";
    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string publicKey = Wallet::getPublicRsaKey(walletPathMth, address.toStdString());

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, publicKey);
    });
}

void JavascriptWrapper::encryptMessage(QString requestId, QString publicKey, QString message) {
    LOG << "encrypt message";

    const QString JS_NAME_RESULT = "encryptMessageResultJs";
    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string answer = Wallet::encryptMessage(publicKey.toStdString(), message.toStdString());

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, answer);
    });
}

void JavascriptWrapper::decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex) {
    LOG << "decrypt message";

    const QString JS_NAME_RESULT = "decryptMessageResultJs";
    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string message = Wallet::decryptMessage(walletPathMth, addr.toStdString(), password.toStdString(), encryptedMessageHex.toStdString());

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, message);
    });
}

////////////////
/// ETHEREUM ///
////////////////

void JavascriptWrapper::createWalletEth(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletEthResultJs";

    LOG << "Create wallet eth " << requestId;

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = EthWallet::genPrivateKey(walletPathEth, password.toStdString());

        return makeJsFuncParams(JS_NAME_RESULT, EthWallet::getFullPath(walletPathEth, address), TypedException(), requestId, address);
    });

    LOG << "Create eth wallet ok " << requestId;
}

void JavascriptWrapper::signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data) {
    const QString JS_NAME_RESULT = "signMessageEthResultJs";

    LOG << "Sign message eth";

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        EthWallet wallet(walletPathEth, address.toStdString(), password.toStdString());
        const std::string result = wallet.SignTransaction(
            nonce.toStdString(),
            gasPrice.toStdString(),
            gasLimit.toStdString(),
            to.toStdString(),
            value.toStdString(),
            data.toStdString()
        );

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, result);
    });
}

void JavascriptWrapper::checkAddressEth(QString requestId, QString address) {
    LOG << "Check address eth " << address;
    const QString JS_NAME_RESULT = "checkAddressEthResultJs";
    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        try {
            EthWallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, QString("not valid"));
        } catch (...) {
            throw;
        }

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, QString("ok"));
    });
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
        LOG << "get eth wallets json " << jsonStr;
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
        LOG << "get eth wallets json " << jsonStr;
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
    const QString JS_NAME_RESULT = "getOnePrivateKeyEthResultJs";

    LOG << "get one private key eth";

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = EthWallet::getOneKey(walletPathEth, keyName.toStdString());

        QString result = QString::fromStdString(privKey);
        result.replace("\"", "\\\"");
        result.replace("\n", "\\n");

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, result);
    });
}

void JavascriptWrapper::savePrivateKeyEth(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key eth";

        EthWallet::savePrivateKey(walletPathEth, privateKey.toStdString(), password.toStdString());

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, "ok");
    });
}

///////////////
/// BITCOIN ///
///////////////

void JavascriptWrapper::createWalletBtcPswd(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletBtcResultJs";

    LOG << "Create wallet btc " << requestId;

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = BtcWallet::genPrivateKey(walletPathBtc, password).first;

        return makeJsFuncParams(JS_NAME_RESULT, BtcWallet::getFullPath(walletPathBtc, address), TypedException(), requestId, address);
    });

    LOG << "Create btc wallet ok " << requestId;
}

void JavascriptWrapper::createWalletBtc(QString requestId) {
    createWalletBtcPswd(requestId, "");
}

void JavascriptWrapper::checkAddressBtc(QString requestId, QString address) {
    LOG << "Check address btc " << address;
    const QString JS_NAME_RESULT = "checkAddressBtcResultJs";
    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        try {
            BtcWallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, QString("not valid"));
        } catch (...) {
            throw;
        }

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, QString("ok"));
    });
}

// deprecated
void JavascriptWrapper::signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
    const QString JS_NAME_RESULT = "signMessageBtcResultJs";

    LOG << "Sign message btc";

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
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

        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        BtcWallet wallet(walletPathBtc, address.toStdString(), password);
        size_t estimateComissionInSatoshiInt = 0;
        if (!estimateComissionInSatoshi.isEmpty()) {
            CHECK(isDecimal(estimateComissionInSatoshi.toStdString()), "Not hex number value");
            estimateComissionInSatoshiInt = std::stoll(estimateComissionInSatoshi.toStdString());
        }
        const auto resultPair = wallet.buildTransaction(btcInputs, estimateComissionInSatoshiInt, value.toStdString(), fees.toStdString(), toAddress.toStdString());
        const std::string &result = resultPair.first;

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, result);
    });
}

void JavascriptWrapper::signMessageBtcPswdUsedUtxos(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees, QString jsonUsedUtxos) {
    const QString JS_NAME_RESULT = "signMessageBtcUsedUtxosResultJs";

    LOG << "Sign message btc utxos ";

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
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
        const std::string &result = resultPair.first;
        const std::set<std::string> &thisUsedTxs = resultPair.second;
        usedUtxos.insert(thisUsedTxs.begin(), thisUsedTxs.end());

        QJsonArray jsonArrayUtxos;
        for (const std::string &r: usedUtxos) {
            jsonArrayUtxos.push_back(QString::fromStdString(r));
        }
        QJsonDocument jsonUtxos(jsonArrayUtxos);

        const std::string &transactionHash = BtcWallet::calcHashNotWitness(result);

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, result, jsonUtxos, transactionHash);
    });
}

// deprecated
void JavascriptWrapper::signMessageBtc(QString requestId, QString address, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
    signMessageBtcPswd(requestId, address, "", jsonInputs, toAddress, value, estimateComissionInSatoshi, fees);
}

QString JavascriptWrapper::getAllBtcWalletsJson() {
    try {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::vector<std::pair<QString, QString>> result = BtcWallet::getAllWalletsInFolder(walletPathBtc);
        const QString jsonStr = makeJsonWallets(result);
        LOG << "get btc wallets json " << jsonStr;
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
        LOG << "get btc wallets json " << jsonStr;
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
    const QString JS_NAME_RESULT = "getOnePrivateKeyBtcResultJs";

    LOG << "get one private key btc";

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = BtcWallet::getOneKey(walletPathBtc, keyName.toStdString());

        QString result = QString::fromStdString(privKey);
        result.replace("\n", "\\n");

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, result);
    });
}

void JavascriptWrapper::savePrivateKeyBtc(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key btc";

        BtcWallet::savePrivateKey(walletPathBtc, privateKey.toStdString(), password);

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, "ok");
    });
}

void JavascriptWrapper::savePrivateKeyAny(QString requestId, QString privateKey, QString password) {
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
}

//////////////
/// COMMON ///
//////////////

void JavascriptWrapper::updateAndReloadApplication() {
    const QString JS_NAME_RESULT = "reloadApplicationJs";

    LOG << "Reload application ";

    apiVrapper(JS_NAME_RESULT, [&, this]() {
        updateAndRestart();

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), "Ok");
    });
}

void JavascriptWrapper::qtOpenInBrowser(QString url) {
    LOG << "Open another url " << url;
    QDesktopServices::openUrl(QUrl(url));
}

void JavascriptWrapper::getWalletFolders() {
    LOG << "getWalletFolders ";
    const QString JS_NAME_RESULT = "walletFoldersJs";
    apiVrapper(JS_NAME_RESULT, [&, this]() {
        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), walletDefaultPath, walletPath, userName);
    });
}

bool JavascriptWrapper::migrateKeysToPath(QString newPath) {
    LOG << "Migrate keys to path " << newPath;

    const QString prevPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_PREV_PATH);

    copyRecursively(makePath(prevPath, WALLET_PATH_ETH), makePath(newPath, WALLET_PATH_ETH), false);
    copyRecursively(makePath(prevPath, WALLET_PATH_BTC), makePath(newPath, WALLET_PATH_BTC), false);
    copyRecursively(makePath(prevPath, WALLET_PATH_MTH), makePath(newPath, WALLET_PATH_MTH), false);
    copyRecursively(makePath(prevPath, WALLET_PATH_TMH), makePath(newPath, WALLET_PATH_TMH), false);
    copyRecursively(prevPath, makePath(newPath, WALLET_PATH_TMH), false);

    return true;
}

void JavascriptWrapper::setPaths(QString newPatch, QString newUserName) {
    const QString JS_NAME_RESULT = "setPathsJs";

    apiVrapper(JS_NAME_RESULT, QString("Not ok"), [&, this]() {
        userName = newUserName;
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
        setPathToWallet(walletPathMth, WALLET_PATH_MTH, "mhc");
        setPathToWallet(walletPathTmh, WALLET_PATH_TMH, "tmh");

        walletPathOldTmh = makePath(walletPath, WALLET_PATH_TMH_OLD);
        LOG << "Wallets path " << walletPath;

        QDir oldTmhPath(walletPathOldTmh);
        if (oldTmhPath.exists()) {
            copyRecursively(walletPathOldTmh, walletPathTmh, true);
            oldTmhPath.removeRecursively();
        }

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), QString("Ok"));
    });
}

QString JavascriptWrapper::openFolderDialog(QString beginPath, QString caption) {
    const QString dir = QFileDialog::getExistingDirectory(widget_, caption, beginPath, QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    LOG << "choised dir " << dir;
    return dir;
}

void JavascriptWrapper::exitApplication() {
    QApplication::exit(SIMPLE_EXIT);
}

void JavascriptWrapper::restartBrowser() {
    LOG << "Restart browser";
    QApplication::exit(RESTART_BROWSER);
}

QString JavascriptWrapper::backupKeys(QString caption) {
    try {
        LOG << "Backup keys";
        const QString beginPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), "backup.zip");
        const QString file = QFileDialog::getSaveFileName(widget_, caption, beginPath);
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
    const QString JS_NAME_RESULT = "machineUidJs";

    const QString uid = "\"" + hardwareId + "\"";
    runJs(JS_NAME_RESULT + "(" +
        uid + "" +
        ");"
    );
}

void JavascriptWrapper::setHasNativeToolbarVariable() {
    emit setHasNativeToolbarVariableSig();
}

void JavascriptWrapper::lineEditReturnPressed(QString text) {
    emit lineEditReturnPressedSig(text);
}

void JavascriptWrapper::setCommandLineText(const QString &/*text*/) {
    //emit setCommandLineTextSig(text);
}

void JavascriptWrapper::openFolderInStandartExplored(const QString &folder) {
    QDesktopServices::openUrl(QUrl::fromLocalFile(folder));
}

void JavascriptWrapper::openWalletPathInStandartExplorer() {
    openFolderInStandartExplored(walletPath);
}

void JavascriptWrapper::setPagesMapping(QString mapping) {
    emit setMappingsSig(mapping);
}

void JavascriptWrapper::getIpsServers(QString requestId, QString type, int length, int count) {
    const QString JS_NAME_RESULT = "getIpsServersJs";

    LOG << "get ips servers " << requestId;

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        const std::vector<QString> result = nsLookup.getRandom(type, length, count);

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

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, resultStr);
    });

    LOG << "get ips servers ok " << requestId;
}

void JavascriptWrapper::setUserName(const QString &userName) {
    emit setUserNameSig(userName);
}

void JavascriptWrapper::saveFileFromUrl(QString url, QString saveFileWindowCaption, QString fileName, bool openAfterSave) {
BEGIN_SLOT_WRAPPER
    LOG << "Save file from url";
    const QString beginPath = makePath(walletPath, fileName);
    const QString file = QFileDialog::getSaveFileName(widget_, saveFileWindowCaption, beginPath);
    CHECK(!file.isNull() && !file.isEmpty(), "File not changed");

    client.sendMessageGet(url, [this, file, openAfterSave](const std::string &response) {
        CHECK(response != SimpleClient::ERROR_BAD_REQUEST, "Error response");
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

    apiVrapper(JS_NAME_RESULT, requestId, [&, this]() {
        const QString beginPath = makePath(walletPath, fileName);
        const QString file = QFileDialog::getOpenFileName(widget_, openFileWindowCaption, beginPath);
        const std::string fileData = readFileBinary(file);
        const std::string base64Data = toBase64(fileData);

        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, base64Data);
    });
END_SLOT_WRAPPER
}

void JavascriptWrapper::printUrl(QString url, QString printWindowCaption, QString text) {
BEGIN_SLOT_WRAPPER
    LOG << "print url";
    client.sendMessageGet(url, [printWindowCaption, text](const std::string &response) {
        CHECK(response != SimpleClient::ERROR_BAD_REQUEST, "Error response");

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

void JavascriptWrapper::getAppInfo(const QString requestId) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "getAppInfoResultJs";

    LOG << "get app info";

    apiVrapper(JS_NAME_RESULT, requestId, [&, this](){
        const std::string versionString = VERSION_STRING;
        const std::string gitCommit = GIT_CURRENT_SHA1;
        return makeJsFuncParams(JS_NAME_RESULT, TypedException(), requestId, isProductionSetup, versionString, gitCommit);
    });
END_SLOT_WRAPPER
}

void JavascriptWrapper::onDirChanged(const QString &dir) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "directoryChangedResultJs";
    const QDir d(dir);
    for (const FolderWalletInfo &folderInfo: folderWalletsInfos) {
        if (folderInfo.walletPath == d) {
            LOG << "folder changed " << folderInfo.nameWallet << " " << d.absolutePath();
            const auto ret = makeJsFuncParams(JS_NAME_RESULT, TypedException(), d.absolutePath(), folderInfo.nameWallet);
            const QString result = ret.invoke();
            runJs(result);
        }
    }
END_SLOT_WRAPPER
}

void JavascriptWrapper::metaOnline() {
    emit wssClient.sendMessage("{\"app\":\"MetaOnline\"}");
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
        apiVrapper(JS_NAME_RESULT, [&, this](){
            CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
            const QJsonObject data = root.value("data").toObject();
            LOG << "Meta online response: " << QString(QJsonDocument(data).toJson(QJsonDocument::Compact));
            return makeJsFuncParams(JS_NAME_RESULT, TypedException(), QJsonDocument(data));
        });
    }
END_SLOT_WRAPPER
}

void JavascriptWrapper::runJs(const QString &script) {
    emit jsRunSig(script);
}
