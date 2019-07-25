#include "WalletsJavascript.h"

#include "Log.h"
#include "check.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/WrapperJavascriptImpl.h"

#include "Wallets.h"
#include "BtcWallet.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

SET_LOG_NAMESPACE("WLTS");

namespace wallets {

static QString makeJsonWallets(const std::vector<std::pair<QString, QString>> &wallets) {
    QJsonArray jsonArray;
    for (const auto &r: wallets) {
        jsonArray.push_back(r.first);
    }
    QJsonDocument json(jsonArray);
    return json.toJson(QJsonDocument::Compact);
}

WalletsJavascript::WalletsJavascript(Wallets &wallets, QObject *parent)
    : WrapperJavascript(false, LOG_FILE)
    , wallets(wallets)
{
    Q_CONNECT(&wallets, &Wallets::watchWalletsAdded, this, &WalletsJavascript::onWatchWalletsCreated);
}

///////////
/// MHC ///
///////////

void WalletsJavascript::onWatchWalletsCreated(bool isMhc, const std::vector<std::pair<QString, QString>> &created) {
BEGIN_SLOT_WRAPPER
    makeAndRunJsFuncParams("walletsCreateWatchWalletsListResultJs", TypedException(), isMhc, makeJsonWallets(created));
    if (isMhc) { // deprecated
        makeAndRunJsFuncParams("createWatchWalletsListMHCResultJs", TypedException(), QString(""), makeJsonWallets(created));
    } else {
        makeAndRunJsFuncParams("createWatchWalletsListResultJs", TypedException(), QString(""), makeJsonWallets(created));
    }
END_SLOT_WRAPPER
}

void WalletsJavascript::createWallet(bool isMhc, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCreateWalletResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<std::string>(""), JsTypeReturn<std::string>(""), JsTypeReturn<std::string>(""), JsTypeReturn<std::string>(""));

    LOG << "Create wallet " << isMhc;

    wrapOperation([&, this](){
        emit wallets.createWallet(isMhc, password, wallets::Wallets::CreateWalletCallback([makeFunc, isMhc](const QString &fullPath, const std::string &pubkey, const std::string &address, const std::string &exampleMessage, const std::string &sign){
            LOG << "Create wallet ok " << isMhc << " " << address;
            makeFunc.func(TypedException(), fullPath, pubkey, address, exampleMessage, sign);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::createWalletWatch(bool isMhc, const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCreateWatchWalletResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<QString>(""));

    LOG << "Create wallet watch " << isMhc << " " << address;

    wrapOperation([&, this](){
        emit wallets.createWatchWallet(isMhc, address, wallets::Wallets::CreateWatchWalletCallback([makeFunc, isMhc, address](const QString &fullPath){
            LOG << "Create wallet watch ok " << isMhc << " " << address;
            makeFunc.func(TypedException(), fullPath, address);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::removeWalletWatch(bool isMhc, const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsRemoveWatchWalletResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    LOG << "Remove wallet watch " << isMhc << " " << address;

    wrapOperation([&, this](){
        emit wallets.removeWatchWallet(isMhc, address, wallets::Wallets::RemoveWatchWalletCallback([makeFunc, isMhc, address](){
            LOG << "Remove wallet watch ok " << isMhc << " " << address;
            makeFunc.func(TypedException(), address);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::checkWalletExist(bool isMhc, const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCheckWalletExistResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<bool>(false));

    wrapOperation([&, this](){
        emit wallets.checkWalletExist(isMhc, address, wallets::Wallets::CheckWalletExistCallback([makeFunc, address](bool isExist){
            makeFunc.func(TypedException(), address, isExist);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::checkWalletPassword(bool isMhc, const QString &address, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCheckWalletPasswordResultJs";

    LOG << "Check wallet password " << address << " " << isMhc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.checkWalletPassword(isMhc, address, password, wallets::Wallets::CheckWalletPasswordCallback([makeFunc, address, isMhc](bool result){
            LOG << "Check wallet password ok " << address << " " << isMhc;
            makeFunc.func(TypedException(), address, result ? "Ok" : "Not ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::checkWalletAddress(const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCheckWalletAddressResultJs";

    LOG << "Check wallet address " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.checkAddress(address, wallets::Wallets::CheckAddressCallback([makeFunc, address](bool result){
            LOG << "Check wallet address ok " << address;
            makeFunc.func(TypedException(), address, result ? "Ok" : "Not ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::createContractAddress(const QString &address, int nonce) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCreateContractAddressResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.createContractAddress(address, nonce, wallets::Wallets::CreateContractAddressCallback([makeFunc, address](const QString &result){
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::signMessage(bool isMhc, const QString &address, const QString &text, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSignMessageResultJs";

    LOG << "Sign message " << isMhc << " " << address << " " << text;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.signMessage(isMhc, address, text, password, wallets::Wallets::SignMessageCallback([makeFunc, isMhc, address](const QString &signature, const QString &pubkey){
            LOG << "Sign message ok " << isMhc << " " << address;
            makeFunc.func(TypedException(), signature, pubkey);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::signMessage2(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSignMessage2ResultJs";

    LOG << "Sign message2 " << isMhc << " " << address << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << dataHex;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<QString>(""), JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.signMessage2(isMhc, address, password, toAddress, value, fee, nonce, dataHex, wallets::Wallets::SignMessage2Callback([makeFunc, isMhc, address](const QString &signature, const QString &pubkey, const QString &tx){
            LOG << "Sign message2 ok " << isMhc << " " << address;
            makeFunc.func(TypedException(), signature, pubkey, tx);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::signAndSendMessage(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &nonce, const QString &dataHex, const QString &paramsJson) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSignAndSendMessageResultJs";

    LOG << "Sign message3 " << isMhc << " " << address << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << dataHex;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.signAndSendMessage(isMhc, address, password, toAddress, value, fee, nonce, dataHex, paramsJson, wallets::Wallets::SignAndSendMessageCallback([makeFunc, isMhc, address](bool success){
            LOG << "Sign message3 ok " << isMhc << " " << address;
            if (success) {
                makeFunc.func(TypedException(), "Ok");
            } else {
                makeFunc.func(TypedException(), "Not ok");
            }
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::signAndSendMessageDelegate(bool isMhc, const QString &address, const QString &password, const QString &toAddress, const QString &value, const QString &fee, const QString &valueDelegate, const QString &nonce, bool isDelegate, const QString &paramsJson) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSignAndSendMessageDelegateResultJs";

    LOG << "Sign message delegate " << isMhc << " " << address << " " << toAddress << " " << value << " " << fee << " " << nonce << " " << isDelegate << " " << valueDelegate;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.signAndSendMessageDelegate(isMhc, address, password, toAddress, value, fee, valueDelegate, nonce, isDelegate, paramsJson, wallets::Wallets::SignAndSendMessageCallback([makeFunc, isMhc, address](bool success){
            LOG << "Sign message delegate ok " << isMhc << " " << address;
            if (success) {
                makeFunc.func(TypedException(), "Ok");
            } else {
                makeFunc.func(TypedException(), "Not ok");
            }
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::getOnePrivateKey(bool isMhc, const QString &address, bool isCompact) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsGetOnePrivateKeyResultJs";

    LOG << "Get private key " << isMhc << " " << address << " " << isCompact;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.getOnePrivateKey(isMhc, address, isCompact, wallets::Wallets::GetPrivateKeyCallback([makeFunc, address](const QString &result){
            LOG << "Get private key ok " << address;
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::savePrivateKey(bool isMhc, const QString &privateKey, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSavePrivateKeyResultJs";

    LOG << "Save private key ";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.savePrivateKey(isMhc, privateKey, password, wallets::Wallets::SavePrivateKeyCallback([makeFunc](bool result){
            LOG << "Save private key ok ";
            if (result) {
                makeFunc.func(TypedException(), "ok");
            } else {
                makeFunc.func(TypedException(), "Not ok");
            }
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::saveRawPrivateKey(bool isMhc, const QString &rawPrivateKey, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSaveRawPrivateKeyResultJs";

    LOG << "Save raw private key ";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.saveRawPrivateKey(isMhc, rawPrivateKey, password, wallets::Wallets::SaveRawPrivateKeyCallback([makeFunc](const QString &address){
            LOG << "Save raw private key ok " << address;
            makeFunc.func(TypedException(), address);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::getRawPrivateKey(bool isMhc, const QString &address, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsGetRawPrivateKeyResultJs";

    LOG << "Get raw private key " << isMhc << " " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.getRawPrivateKey(isMhc, address, password, wallets::Wallets::GetRawPrivateKeyCallback([makeFunc](const QString &result){
            LOG << "Get raw private key ok ";
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

///////////
/// RSA ///
///////////

void WalletsJavascript::createRsaKey(bool isMhc, const QString &address, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCreateRsaKeyResultJs";

    LOG << "Create rsa key " << isMhc << " " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.createRsaKey(isMhc, address, password, wallets::Wallets::CreateRsaKeyCallback([makeFunc](const QString &publicKey){
            LOG << "Create rsa key ok ";
            makeFunc.func(TypedException(), publicKey);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::getRsaPublicKey(bool isMhc, const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsGetRsaPublicKeyResultJs";

    LOG << "Get rsa public key " << isMhc << " " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.getRsaPublicKey(isMhc, address, wallets::Wallets::GetRsaPublicKeyCallback([makeFunc](const QString &publicKey){
            makeFunc.func(TypedException(), publicKey);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::copyRsaKey(bool isMhc, const QString &address, const QString &pathPub, const QString &pathPriv) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCopyRsaKeyResultJs";

    LOG << "Copy rsa key " << isMhc << " " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.copyRsaKey(isMhc, address, pathPub, pathPriv, wallets::Wallets::CopyRsaKeyCallback([makeFunc](bool success){
            LOG << "Copy rsa key ok";
            if (success) {
                makeFunc.func(TypedException(), "Ok");
            } else {
                makeFunc.func(TypedException(), "Not ok");
            }
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::copyRsaKeyToFolder(bool isMhc, const QString &address, const QString &path) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCopyRsaKeyToFolderResultJs";

    LOG << "Copy rsa key to folder " << isMhc << " " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.copyRsaKeyToFolder(isMhc, address, path, wallets::Wallets::CopyRsaKeyCallback([makeFunc](bool success){
            LOG << "Copy rsa key to folder ok";
            if (success) {
                makeFunc.func(TypedException(), "Ok");
            } else {
                makeFunc.func(TypedException(), "Not ok");
            }
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

////////////////
/// ETHEREUM ///
////////////////

void WalletsJavascript::createEthKey(const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCreateEthKeyResultJs";

    LOG << "Create eth key ";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.createEthKey(password, wallets::Wallets::CreateEthKeyCallback([makeFunc](const QString &address, const QString &fullPath){
            LOG << "Create eth key ok " << address;
            makeFunc.func(TypedException(), address, fullPath);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::signMessageEth(const QString &address, const QString &password, const QString &nonce, const QString &gasPrice, const QString &gasLimit, const QString &to, const QString &value, const QString &data) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSignMessageEthResultJs";

    LOG << "Sign message eth " << address << " " << nonce << " " << gasPrice << " " << gasLimit << " " << to << " " << value << " " << data;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.signMessageEth(address, password, nonce, gasPrice, gasLimit, to, value, data, wallets::Wallets::SignMessageEthCallback([makeFunc](const QString &result){
            LOG << "Sign message eth ok ";
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::checkAddressEth(const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCheckAddressEthResultJs";

    LOG << "Check address eth " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.checkAddressEth(address, wallets::Wallets::CheckAddressCallback([makeFunc](bool success){
            makeFunc.func(TypedException(), success ? "Ok" : "Not ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::savePrivateKeyEth(const QString &privateKey, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSavePrivateKeyEthResultJs";

    LOG << "Save private eth key ";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.savePrivateKeyEth(privateKey, password, wallets::Wallets::SavePrivateKeyCallback([makeFunc](bool success){
            makeFunc.func(TypedException(), success ? "Ok" : "Not ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::getOnePrivateKeyEth(const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsGetOnePrivateKeyEthResultJs";

    LOG << "Get private key eth " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.getOnePrivateKeyEth(address, wallets::Wallets::GetPrivateKeyCallback([makeFunc](const QString &result){
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

///////////
/// BTC ///
///////////

void WalletsJavascript::createBtcKey(const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCreateBtcKeyResultJs";

    LOG << "Create btc key ";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.createBtcKey(password, wallets::Wallets::CreateBtcKeyCallback([makeFunc](const QString &address, const QString &fullPath){
            LOG << "Create btc key ok " << address;
            makeFunc.func(TypedException(), address, fullPath);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::checkAddressBtc(const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsCheckAddressBtcResultJs";

    LOG << "Check address btc " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.checkAddressBtc(address, wallets::Wallets::CheckAddressCallback([makeFunc](bool success){
            makeFunc.func(TypedException(), success ? "Ok" : "Not ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::signMessageBtcUsedUtxos(const QString &address, const QString &password, const QString &jsonInputs, const QString &toAddress, const QString &value, const QString &estimateComissionInSatoshi, const QString &fees, const QString &jsonUsedUtxos) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSignMessageBtcResultJs";

    LOG << "Sign message btc utxos " << address << " " << toAddress << " " << value << " " << estimateComissionInSatoshi << " " << fees;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<QJsonDocument>(QJsonDocument()), JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
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

        emit wallets.signMessageBtcUsedUtxos(address, password, btcInputs, toAddress, value, estimateComissionInSatoshi, fees, usedUtxos, wallets::Wallets::SignMessageBtcCallback([makeFunc](const QString &result, const QString &hash, const std::set<std::string> &usedUtxos){
            LOG << "Sign message btc ok ";
            QJsonArray jsonArrayUtxos;
            for (const std::string &r: usedUtxos) {
                jsonArrayUtxos.push_back(QString::fromStdString(r));
            }
            const QJsonDocument jsonUtxos = QJsonDocument(jsonArrayUtxos);

            makeFunc.func(TypedException(), result, jsonUtxos, hash);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::savePrivateKeyBtc(const QString &privateKey, const QString &password) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsSavePrivateKeyBtcResultJs";

    LOG << "Save private btc key ";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit wallets.savePrivateKeyBtc(privateKey, password, wallets::Wallets::SavePrivateKeyCallback([makeFunc](bool success){
            makeFunc.func(TypedException(), success ? "Ok" : "Not ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletsJavascript::getOnePrivateKeyBtc(const QString &address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "walletsGetOnePrivateKeyBtcResultJs";

    LOG << "Get private key btc " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit wallets.getOnePrivateKeyBtc(address, wallets::Wallets::GetPrivateKeyCallback([makeFunc](const QString &result){
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

} // namespace wallets
