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

void WalletsJavascript::createWallet(bool isMhc, QString password) {
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

} // namespace wallets
