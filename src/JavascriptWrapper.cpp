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

    walletDefaultPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath(WALLET_PATH_DEFAULT);
    LOG << "Wallets default path " << walletPath;

    setPaths(walletDefaultPath, "");

    CHECK(connect(&client, SIGNAL(callbackCall(ReturnCallback)), this, SLOT(onCallbackCall(ReturnCallback))), "not connect callbackCall");
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

        const QString jScript = jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(publicKey) + "\", " +
            "\"" + QString::fromStdString(addr) + "\", " +
            "\"" + QString::fromStdString(exampleMessage) + "\", " +
            "\"" + QString::fromStdString(signature) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\", " +
            "\"" + wallet.getFullPath() + "\"" +
            ");";
        //LOG << jScript.toStdString() << std::endl;
        jsRunSig(jScript);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\", " +
            "\"" + "" + "\"" +
            ");"
        );
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

void JavascriptWrapper::signMessageMHC(QString requestId, QString keyName, QString text, QString password) {
    signMessageMTHS(requestId, keyName, text, password, walletPathMth, "signMessageMHCResultJs");
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

void JavascriptWrapper::signMessageMTHS(QString requestId, QString keyName, QString text, QString password, QString walletPath, QString jsNameResult) {
    LOG << requestId;
    LOG << keyName;
    LOG << text;

    const std::string textStr = text.toStdString();

    const TypedException &exception = apiVrapper([this, &jsNameResult, &requestId, &keyName, &textStr, &password, &walletPath]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        Wallet wallet(walletPath, keyName.toStdString(), password.toStdString());
        std::string publicKey;
        const std::string signature = wallet.sign(textStr, publicKey);

        jsRunSig(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(signature) + "\", " +
            "\"" + QString::fromStdString(publicKey) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

void JavascriptWrapper::getOnePrivateKeyMTHS(QString requestId, QString keyName, bool isCompact, QString walletPath, QString jsNameResult) {
    const TypedException &exception = apiVrapper([this, &jsNameResult, &requestId, &keyName, &isCompact, &walletPath]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = Wallet::getPrivateKey(walletPath, keyName.toStdString(), isCompact);

        QString result = QString::fromStdString(privKey);
        result.replace("\n", "\\n");

        LOG << "Getted private key " << result;

        jsRunSig(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + result + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

void JavascriptWrapper::getOnePrivateKey(QString requestId, QString keyName, bool isCompact) {
    const QString JS_NAME_RESULT = "getOnePrivateKeyResultJs";
    getOnePrivateKeyMTHS(requestId, keyName, isCompact, walletPathTmh, JS_NAME_RESULT);
}

void JavascriptWrapper::getOnePrivateKeyMHC(QString requestId, QString keyName, bool isCompact) {
    const QString JS_NAME_RESULT = "getOnePrivateKeyMHCResultJs";
    getOnePrivateKeyMTHS(requestId, keyName, isCompact, walletPathMth, JS_NAME_RESULT);
}

void JavascriptWrapper::savePrivateKeyMTHS(QString requestId, QString privateKey, QString password, QString walletPath, QString jsNameResult) {
    const TypedException &exception = apiVrapper([this, &jsNameResult, &requestId, &privateKey, &password, &walletPath]() {
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key";

        Wallet::savePrivateKey(walletPath, privateKey.toStdString(), password.toStdString());

        jsRunSig(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + "ok" + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(jsNameResult + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

void JavascriptWrapper::savePrivateKey(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyResultJs";
    savePrivateKeyMTHS(requestId, privateKey, password, walletPathTmh, JS_NAME_RESULT);
}

void JavascriptWrapper::savePrivateKeyMHC(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyMHCResultJs";
    savePrivateKeyMTHS(requestId, privateKey, password, walletPathMth, JS_NAME_RESULT);
}

void JavascriptWrapper::createRsaKey(QString requestId, QString address, QString password) {
    const QString JS_NAME_RESULT = "createRsaKeyResultJs";
    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &address, &requestId, &password]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string publicKey = Wallet::createRsaKey(walletPathMth, address.toStdString(), password.toStdString());

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(publicKey) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
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
}

void JavascriptWrapper::decryptMessage(QString requestId, QString addr, QString password, QString encryptedMessageHex) {
    const QString JS_NAME_RESULT = "decryptMessageResultJs";
    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathMth.isNull() && !walletPathMth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string message = Wallet::decryptMessage(walletPathMth, addr.toStdString(), password.toStdString(), encryptedMessageHex.toStdString());

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(message) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
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
}

////////////////
/// ETHEREUM ///
////////////////

void JavascriptWrapper::createWalletEth(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletEthResultJs";

    LOG << "Create wallet eth " << requestId;

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &password]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = EthWallet::genPrivateKey(walletPathEth, password.toStdString());

        const QString jScript = JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(address) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\", " +
            "\"" + EthWallet::getFullPath(walletPathEth, address) + "\"" +
            ");";
        //LOG << jScript.toStdString() << std::endl;
        jsRunSig(jScript);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\", " +
            "\"" + "" + "\"" +
            ");"
        );
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

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(result) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
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

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + result + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
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
}

void JavascriptWrapper::savePrivateKeyEth(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyEthResultJs";

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &privateKey, &password]() {
        CHECK(!walletPathEth.isNull() && !walletPathEth.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key eth";

        EthWallet::savePrivateKey(walletPathEth, privateKey.toStdString(), password.toStdString());

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "ok" + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
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
}

///////////////
/// BITCOIN ///
///////////////

void JavascriptWrapper::createWalletBtcPswd(QString requestId, QString password) {
    const QString JS_NAME_RESULT = "createWalletBtcResultJs";

    LOG << "Create wallet btc " << requestId;

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &password]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");
        const std::string address = BtcWallet::genPrivateKey(walletPathBtc, password).first;

        const QString jScript = JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(address) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\", " +
            "\"" + BtcWallet::getFullPath(walletPathBtc, address) + "\"" +
            ");";
        //LOG << jScript.toStdString() << std::endl;
        jsRunSig(jScript);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\", " +
            "\"" + "" + "\"" +
            ");"
        );
    }

    LOG << "Create btc wallet ok " << requestId;
}

void JavascriptWrapper::createWalletBtc(QString requestId) {
    createWalletBtcPswd(requestId, "");
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

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + QString::fromStdString(result) + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
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

    LOG << "get one private key eth";

    const TypedException &exception = apiVrapper([&, this]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");

        const std::string privKey = BtcWallet::getOneKey(walletPathBtc, keyName.toStdString());

        QString result = QString::fromStdString(privKey);
        result.replace("\n", "\\n");

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + result + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
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
}

void JavascriptWrapper::savePrivateKeyBtc(QString requestId, QString privateKey, QString password) {
    const QString JS_NAME_RESULT = "savePrivateKeyBtcResultJs";

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &privateKey, &password]() {
        CHECK(!walletPathBtc.isNull() && !walletPathBtc.isEmpty(), "Incorrect path to wallet: empty");

        LOG << "Save private key eth";

        BtcWallet::savePrivateKey(walletPathBtc, privateKey.toStdString(), password);

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "ok" + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
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
}

//////////////
/// COMMON ///
//////////////

void JavascriptWrapper::updateAndReloadApplication() {
    const QString JS_NAME_RESULT = "reloadApplicationJs";

    LOG << "Reload application ";

    const TypedException &exception = apiVrapper([&, this]() {
        updateAndRestart();

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + "Ok" + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + "Not ok" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }
}

void JavascriptWrapper::qtOpenInBrowser(QString url) {
    LOG << "Open another url " << url;
    QDesktopServices::openUrl(QUrl(url));
}

void JavascriptWrapper::getWalletFolders() {
    LOG << "getWalletFolders ";
    const QString JS_NAME_RESULT = "walletFoldersJs";
    jsRunSig(JS_NAME_RESULT + "(" +
        "\"" + walletDefaultPath + "\", " +
        "\"" + walletPath + "\", " +
        "\"" + userName + "\", " +
        QString::fromStdString(std::to_string(0)) + ", " +
        "\"" + QString::fromStdString("") + "\"" +
        ");"
    );
}

bool JavascriptWrapper::migrateKeysToPath(QString newPath) {
    LOG << "Migrate keys to path " << newPath;

    const QString prevPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath(WALLET_PREV_PATH);

    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_ETH), QDir(newPath).filePath(WALLET_PATH_ETH), false);
    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_BTC), QDir(newPath).filePath(WALLET_PATH_BTC), false);
    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_MTH), QDir(newPath).filePath(WALLET_PATH_MTH), false);
    copyRecursively(QDir(prevPath).filePath(WALLET_PATH_TMH), QDir(newPath).filePath(WALLET_PATH_TMH), false);
    copyRecursively(prevPath, QDir(newPath).filePath(WALLET_PATH_TMH), false);

    return true;
}

void JavascriptWrapper::setPaths(QString newPatch, QString newUserName) {
    const QString JS_NAME_RESULT = "setPathsJs";

    const TypedException &exception = apiVrapper([&, this]() {
        userName = newUserName;
        walletPath = newPatch;
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
        createFolder(walletPath);
        walletPathEth = QDir(walletPath).filePath(WALLET_PATH_ETH);
        createFolder(walletPathEth);
        walletPathBtc = QDir(walletPath).filePath(WALLET_PATH_BTC);
        createFolder(walletPathBtc);
        walletPathMth = QDir(walletPath).filePath(WALLET_PATH_MTH);
        createFolder(walletPathMth);
        walletPathTmh = QDir(walletPath).filePath(WALLET_PATH_TMH);
        createFolder(walletPathTmh);
        walletPathOldTmh = QDir(walletPath).filePath(WALLET_PATH_TMH_OLD);
        LOG << "Wallets path " << walletPath;

        QDir oldTmhPath(walletPathOldTmh);
        if (oldTmhPath.exists()) {
            copyRecursively(walletPathOldTmh, walletPathTmh, true);
            oldTmhPath.removeRecursively();
        }

        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + "Ok" + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");"
        );
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        jsRunSig(JS_NAME_RESULT + "(" +
            "\"" + "Not ok" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
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

QString JavascriptWrapper::backupKeys(QString caption) {
    try {
        const QString beginPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath("backup.zip");
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
        const QString beginPath = QDir(QStandardPaths::writableLocation(QStandardPaths::HomeLocation)).filePath("backup.zip");
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
    jsRunSig(JS_NAME_RESULT + "(" +
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

    const TypedException &exception = apiVrapper([this, &JS_NAME_RESULT, &requestId, &type, length, count]() {
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

        const QString jScript = JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + resultStr + "\", " +
            QString::fromStdString(std::to_string(TypeErrors::NOT_ERROR)) + ", " +
            "\"" + "" + "\"" +
            ");";
        //LOG << jScript.toStdString() << std::endl;
        jsRunSig(jScript);
    });

    if (exception.numError != TypeErrors::NOT_ERROR) {
        runJs(JS_NAME_RESULT + "(" +
            "\"" + requestId + "\", " +
            "\"" + "" + "\", " +
            QString::fromStdString(std::to_string(exception.numError)) + ", " +
            "\"" + QString::fromStdString(exception.description) + "\"" +
            ");"
        );
    }

    LOG << "get ips servers ok " << requestId;
}

void JavascriptWrapper::setUserName(const QString &userName) {
    emit setUserNameSig(userName);
}

void JavascriptWrapper::saveFileFromUrl(QString url, QString saveFileWindowCaption, QString fileName, bool openAfterSave) {
BEGIN_SLOT_WRAPPER
    const QString beginPath = QDir(walletPath).filePath(fileName);
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

void JavascriptWrapper::printUrl(QString url, QString printWindowCaption, QString text) {
BEGIN_SLOT_WRAPPER
    client.sendMessageGet(url, [this, printWindowCaption, text](const std::string &response) {
        CHECK(response != SimpleClient::ERROR_BAD_REQUEST, "Error response");

        QImage image;
        image.loadFromData((const unsigned char*)response.data(), (int)response.size());

        QPrinter printer;

        QPrintDialog *dialog = new QPrintDialog(&printer);
        dialog->setWindowTitle(printWindowCaption);

        if (dialog->exec() != QDialog::Accepted)
            return -1;

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

void JavascriptWrapper::runJs(const QString &script) {
    emit jsRunSig(script);
}
