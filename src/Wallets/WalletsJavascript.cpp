#include "WalletsJavascript.h"

#include "Log.h"
#include "check.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/WrapperJavascriptImpl.h"

#include "Wallets.h"

#include <QJsonArray>
#include <QJsonDocument>

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
            LOG << "Sign message ok " << isMhc << " " << address;
            makeFunc.func(TypedException(), signature, pubkey, tx);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

} // namespace wallets
