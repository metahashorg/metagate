#include "JavascriptWrapper.h"

#include <iostream>
#include <fstream>
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

#include "unzip.h"
#include "check.h"
#include "StopApplication.h"
#include "duration.h"
#include "Log.h"
#include "utils.h"
#include "TypedException.h"
#include "SlotWrapper.h"
#include "platform.h"

#include "machine_uid.h"

const static QString WALLET_PREV_PATH = ".metahash_wallets/";
const static QString WALLET_PATH_DEFAULT = ".metahash_wallets/";
const static QString WALLET_PATH_ETH = "eth/";
const static QString WALLET_PATH_BTC = "btc/";
const static QString WALLET_PATH_MTH = "mhc/";
const static QString WALLET_PATH_TMH_OLD = "mth/";
const static QString WALLET_PATH_TMH = "tmh/";

JavascriptWrapper::JavascriptWrapper(NsLookup &nsLookup, QObject */*parent*/)
    : nsLookup(nsLookup)
{
    hardwareId = QString::fromStdString(::getMachineUid());

    walletDefaultPath = makePath(QStandardPaths::writableLocation(QStandardPaths::HomeLocation), WALLET_PATH_DEFAULT);
    LOG << "Wallets default path " << walletPath;

    setPaths(walletDefaultPath, "");

    CHECK(connect(&client, SIGNAL(callbackCall(ReturnCallback)), this, SLOT(onCallbackCall(ReturnCallback))), "not connect callbackCall");

    CHECK(connect(&fileSystemWatcher, SIGNAL(directoryChanged(const QString&)), this, SLOT(onDirChanged(const QString&))), "not connect fileSystemWatcher");
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
static TypedException apiVrapper(const Function &func) {
    try {
        func();
        return TypedException(TypeErrors::NOT_ERROR, "");
    } catch (const TypedException &e) {
        LOG << "Error " << std::to_string(e.numError) << ". " << e.description;
        return e;
    } catch (const Exception &e) {
        LOG << "Error " << e;
        return TypedException(TypeErrors::OTHER_ERROR, e);
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
        return TypedException(TypeErrors::OTHER_ERROR, e.what());
    } catch (...) {
        LOG << "Unknown error";
        return TypedException(TypeErrors::OTHER_ERROR, "Unknown error");
    }
}

static QString toJsString(const QString &arg) {
    return "\"" + arg + "\"";
}

static QString toJsString(const std::string &arg) {
    return "\"" + QString::fromStdString(arg) + "\"";
}

static QString toJsString(const char *arg) {
    return "\"" + QString(arg) + "\"";
}

static QString toJsString(const int &arg) {
    return QString::fromStdString(std::to_string(arg));
}

static QString toJsString(bool arg) {
    if (arg) {
        return "true";
    } else {
        return "false";
    }
}

static QString toJsString(const size_t &arg) {
    return QString::fromStdString(std::to_string(arg));
}

template<typename Arg>
static QString append(const Arg &arg) {
    return toJsString(arg);
}

template<typename Arg, typename... Args>
static QString append(const Arg &arg, Args&& ...args) {
    return toJsString(arg) + ", " + append(std::forward<Args>(args)...);
}

template<typename... Args>
void JavascriptWrapper::runJsFunc(const QString &function, const QString &lastArg, const TypedException &exception, Args&& ...args) {
    QString jScript = function + "(";
    jScript += append(std::forward<Args>(args)...) + ", ";
    jScript += append(exception.numError, exception.description);
    if (!lastArg.isNull() && !lastArg.isEmpty()) {
        jScript += ", \"" + lastArg + "\"";
    }
    jScript += ");";
    //LOG << "JScript " << jScript;
    runJs(jScript);
}

template<typename... Args>
void JavascriptWrapper::runJsFunc(const QString &function, const TypedException &exception, Args&& ...args) {
    runJsFunc(function, "", exception, std::forward<Args>(args)...);
}

////////////////
/// METAHASH ///
////////////////

void JavascriptWrapper::createWalletMTHS(QString requestId, QString password, QString walletPath, QString jsNameResult) {
    LOG << "Create wallet " << requestId;

    const TypedException &exception = apiVrapper([this, &jsNameResult, &requestId, &password, &walletPath]() {
        std::string publicKey;
        std::string addr;
        const std::string exampleMessage = "Example message " + std::to_string(rand());
        std::string signature;

        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::createWallet(walletPath, password.toStdString(), publicKey, addr);

        publicKey.clear();
        Wallet wallet(walletPath, addr, password.toStdString());
        signature = wallet.sign(exampleMessage, publicKey);

        runJsFunc(jsNameResult, wallet.getFullPath(), TypedException(), requestId, publicKey, addr, exampleMessage, signature);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(jsNameResult, "", exception, requestId, "", "", "", "");
    }

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
    const TypedException &exception = apiVrapper([&, this]() {
        try {
            Wallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            runJsFunc(JS_NAME_RESULT, TypedException(), requestId, "not valid");
        } catch (...) {
            throw;
        }

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, "ok");
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString text, QString password, QString walletPath, QString jsNameResult) {
    LOG << "Sign message " << requestId << keyName << text;

    const std::string textStr = text.toStdString();

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        const std::string signature = wallet.sign(textStr, publicKey);

        runJsFunc(jsNameResult, TypedException(), requestId, signature, publicKey);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(jsNameResult, exception, requestId, "", "");
    }
}

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString password, QString toAddress, QString value, QString fee, QString nonce, QString data, QString walletPath, QString jsNameResult) {
    LOG << "Sign message " << requestId << keyName << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << data;

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        std::string tx;
        std::string signature;
        bool tmp;
        wallet.sign(toAddress.toStdString(), value.toULongLong(&tmp, 10), fee.toULongLong(&tmp, 10), nonce.toULongLong(&tmp, 10), data.toStdString(), tx, signature, publicKey);

        runJsFunc(jsNameResult, TypedException(), requestId, signature, publicKey, tx);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(jsNameResult, exception, requestId, "", "", "");
    }
}

void JavascriptWrapper::getOnePrivateKeyMTHS(QString requestId, QString keyName, bool isCompact, QString walletPath, QString jsNameResult, bool isTmh) {
    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = Wallet::getPrivateKey(walletPath, keyName.toStdString(), isCompact, isTmh);

        QString result = QString::fromStdString(privKey);
        result.replace("\n", "\\n");

        LOG << "Getted private key";

        runJsFunc(jsNameResult, TypedException(), requestId, result);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(jsNameResult, exception, requestId, "");
    }
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
    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key";

        Wallet::savePrivateKey(walletPath, privateKey.toStdString(), password.toStdString());

        runJsFunc(jsNameResult, TypedException(), requestId, "ok");
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(jsNameResult, exception, requestId, "");
    }
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
    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        Wallet::createRsaKey(walletPathMth, address.toStdString(), password.toStdString());
        const std::string publicKey = Wallet::getPublicKeyMessage(walletPathMth, address.toStdString());

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, publicKey);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

void JavascriptWrapper::getRsaPublicKey(QString requestId, QString address) {
    LOG << "Get rsa key";

    const QString JS_NAME_RESULT = "getRsaPublicKeyResultJs";
    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string publicKey = Wallet::getPublicKeyMessage(walletPathMth, address.toStdString());

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, publicKey);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

void JavascriptWrapper::encryptMessage(QString requestId, QString publicKey, QString message) {
    LOG << "encrypt message";

    const QString JS_NAME_RESULT = "encryptMessageResultJs";
    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string answer = Wallet::encryptMessage(publicKey.toStdString(), message.toStdString());

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, answer);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

void JavascriptWrapper::decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex) {
    LOG << "decrypt message";

    const QString JS_NAME_RESULT = "decryptMessageResultJs";
    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string message = Wallet::decryptMessage(walletPathMth, addr.toStdString(), password.toStdString(), encryptedMessageHex.toStdString());

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, message);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

////////////////
/// ETHEREUM ///
////////////////

void JavascriptWrapper::createWalletEth(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletEthResultJs";

    LOG << "Create wallet eth " << requestId;

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = EthWallet::genPrivateKey(walletPathEth, password.toStdString());

        runJsFunc(JS_NAME_RESULT, EthWallet::getFullPath(walletPathEth, address), TypedException(), requestId, address);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, "", exception, requestId, "");
    }

    LOG << "Create eth wallet ok " << requestId;
}

void JavascriptWrapper::signMessageEth(QString requestId, QString address, QString password, QString nonce, QString gasPrice, QString gasLimit, QString to, QString value, QString data) {
    const QString JS_NAME_RESULT = "signMessageEthResultJs";

    LOG << "Sign message eth";

    const TypedException &exception = apiVrapper([&, this]() {
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

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, result);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

void JavascriptWrapper::checkAddressEth(QString requestId, QString address) {
    LOG << "Check address eth " << address;
    const QString JS_NAME_RESULT = "checkAddressEthResultJs";
    const TypedException &exception = apiVrapper([&, this]() {
        try {
            EthWallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            runJsFunc(JS_NAME_RESULT, TypedException(), requestId, "not valid");
        } catch (...) {
            throw;
        }

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, "ok");
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
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

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = EthWallet::getOneKey(walletPathEth, keyName.toStdString());

        QString result = QString::fromStdString(privKey);
        result.replace("\"", "\\\"");
        result.replace("\n", "\\n");

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, result);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

void JavascriptWrapper::savePrivateKeyEth(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key eth";

        EthWallet::savePrivateKey(walletPathEth, privateKey.toStdString(), password.toStdString());

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, "ok");
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

///////////////
/// BITCOIN ///
///////////////

void JavascriptWrapper::createWalletBtcPswd(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletBtcResultJs";

    LOG << "Create wallet btc " << requestId;

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = BtcWallet::genPrivateKey(walletPathBtc, password).first;

        runJsFunc(JS_NAME_RESULT, BtcWallet::getFullPath(walletPathBtc, address), TypedException(), requestId, address);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, "", exception, requestId, "");
    }

    LOG << "Create btc wallet ok " << requestId;
}

void JavascriptWrapper::createWalletBtc(QString requestId) {
    createWalletBtcPswd(requestId, "");
}

void JavascriptWrapper::checkAddressBtc(QString requestId, QString address) {
    LOG << "Check address btc " << address;
    const QString JS_NAME_RESULT = "checkAddressBtcResultJs";
    const TypedException &exception = apiVrapper([&, this]() {
        try {
            BtcWallet::checkAddress(address.toStdString());
        } catch (const Exception &e) {
            runJsFunc(JS_NAME_RESULT, TypedException(), requestId, "not valid");
        } catch (...) {
            throw;
        }

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, "ok");
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

void JavascriptWrapper::signMessageBtcPswd(QString requestId, QString address, QString password, QString jsonInputs, QString toAddress, QString value, QString estimateComissionInSatoshi, QString fees) {
    const QString JS_NAME_RESULT = "signMessageBtcResultJs";

    LOG << "Sign message btc";

    const TypedException &exception = apiVrapper([&, this]() {
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
        const std::string result = wallet.buildTransaction(btcInputs, estimateComissionInSatoshiInt, value.toStdString(), fees.toStdString(), toAddress.toStdString());

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, result);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

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

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = BtcWallet::getOneKey(walletPathBtc, keyName.toStdString());

        QString result = QString::fromStdString(privKey);
        result.replace("\n", "\\n");

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, result);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
}

void JavascriptWrapper::savePrivateKeyBtc(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyAnyResultJs";

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key btc";

        BtcWallet::savePrivateKey(walletPathBtc, privateKey.toStdString(), password);

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, "ok");
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
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

    const TypedException &exception = apiVrapper([&, this]() {
        updateAndRestart();

        runJsFunc(JS_NAME_RESULT, TypedException(), "Ok");
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, "Not ok");
    }
}

void JavascriptWrapper::qtOpenInBrowser(QString url) {
    LOG << "Open another url " << url;
    QDesktopServices::openUrl(QUrl(url));
}

void JavascriptWrapper::getWalletFolders() {
    LOG << "getWalletFolders ";
    const QString JS_NAME_RESULT = "walletFoldersJs";
    runJsFunc(JS_NAME_RESULT, TypedException(), walletDefaultPath, walletPath, userName);
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

    const TypedException &exception = apiVrapper([&, this]() {
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

        runJsFunc(JS_NAME_RESULT, TypedException(), "Ok");
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, "Not ok");
    }
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

    const TypedException &exception = apiVrapper([&, this]() {
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

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, resultStr);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }

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

    const TypedException &exception = apiVrapper([&, this]() {
        const QString beginPath = makePath(walletPath, fileName);
        const QString file = QFileDialog::getOpenFileName(widget_, openFileWindowCaption, beginPath);
        const std::string fileData = readFileBinary(file);
        const std::string base64Data = toBase64(fileData);

        runJsFunc(JS_NAME_RESULT, TypedException(), requestId, base64Data);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJsFunc(JS_NAME_RESULT, exception, requestId, "");
    }
END_SLOT_WRAPPER
}

void JavascriptWrapper::printUrl(QString url, QString printWindowCaption, QString text) {
BEGIN_SLOT_WRAPPER
    LOG << "print url";
    client.sendMessageGet(url, [this, printWindowCaption, text](const std::string &response) {
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

    const std::string versionString = VERSION_STRING;
    const std::string gitCommit = GIT_CURRENT_SHA1;
    runJsFunc(JS_NAME_RESULT, TypedException(), requestId, isProductionSetup, versionString, gitCommit);
END_SLOT_WRAPPER
}

void JavascriptWrapper::onDirChanged(const QString &dir) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "directoryChangedResultJs";
    const QDir d(dir);
    for (const FolderWalletInfo &folderInfo: folderWalletsInfos) {
        if (folderInfo.walletPath == d) {
            LOG << "folder changed " << folderInfo.nameWallet << " " << d.absolutePath();
            runJsFunc(JS_NAME_RESULT, TypedException(), d.absolutePath(), folderInfo.nameWallet);
            return;
        }
    }
END_SLOT_WRAPPER
}

void JavascriptWrapper::runJs(const QString &script) {
    emit jsRunSig(script);
}
