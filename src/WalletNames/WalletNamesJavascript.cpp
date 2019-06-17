#include "WalletNamesJavascript.h"

#include "Log.h"
#include "check.h"
#include "SlotWrapper.h"
#include "QRegister.h"

#include "WalletNames.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "WrapperJavascriptImpl.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("WNS");

namespace wallet_names {

static std::vector<WalletInfo> parseWalletNames(const QString &walletsJson) {
    std::vector<WalletInfo> result;

    const QJsonDocument jsonResponse = QJsonDocument::fromJson(walletsJson.toUtf8());
    CHECK(jsonResponse.isArray(), "Incorrect json ");
    const QJsonArray &jsonArr = jsonResponse.array();
    for (const QJsonValue &jsonVal: jsonArr) {
        CHECK(jsonVal.isObject(), "Incorrect json");
        const QJsonObject &walletJson = jsonVal.toObject();

        WalletInfo wallet;
        CHECK(walletJson.contains("address") && walletJson.value("address").isString(), "Incorrect json: address field not found");
        wallet.address = walletJson.value("address").toString();
        CHECK(walletJson.contains("name") && walletJson.value("name").isString(), "Incorrect json: name field not found");
        wallet.name = walletJson.value("name").toString();

        result.emplace_back(wallet);
    }

    return result;
}

WalletNamesJavascript::WalletNamesJavascript(WalletNames &walletNames, QObject *parent)
    : WrapperJavascript(false)
    , manager(walletNames)
{
    CHECK(connect(&manager, &WalletNames::updatedWalletName, this, &WalletNamesJavascript::onUpdatedWalletName), "not connect onUpdatedWalletName");
    CHECK(connect(&manager, &WalletNames::walletsFlushed, this, &WalletNamesJavascript::onWalletsFlushed), "not connect onWalletsFlushed");
}

static int typeToInt(const WalletInfo::Info::Type &type) {
    if (type == WalletInfo::Info::Type::Key) {
        return 1;
    } else if (type == WalletInfo::Info::Type::Watch) {
        return 2;
    } else {
        throwErr("Unknown type");
    }
}

static QJsonDocument walletsToJson(const std::vector<WalletInfo> &thisWallets, const std::vector<WalletInfo> &otherWallets) {
    const auto walletsToJson = [](const std::vector<WalletInfo> &wallets) {
        QJsonArray walletsJson;
        for (const WalletInfo &wallet: wallets) {
            QJsonObject walletJson;
            walletJson.insert("address", wallet.address);
            walletJson.insert("name", wallet.name);
            walletJson.insert("currency", wallet.currentInfo.currency);
            walletJson.insert("type", typeToInt(wallet.currentInfo.type));

            QJsonArray infosJosn;
            for (const WalletInfo::Info &info: wallet.infos) {
                QJsonObject infoJson;
                infoJson.insert("currency", info.currency);
                infoJson.insert("device", info.device);
                infoJson.insert("user", info.user);
                infoJson.insert("type", typeToInt(info.type));

                infosJosn.append(infoJson);
            }
            walletJson.insert("info", infosJosn);

            walletsJson.append(walletJson);
        }
        return walletsJson;
    };

    const QJsonArray thisArrJson = walletsToJson(thisWallets);
    const QJsonArray otherArrJson = walletsToJson(otherWallets);

    QJsonObject result;
    result.insert("this", thisArrJson);
    result.insert("other", otherArrJson);

    return QJsonDocument(result);
}

void WalletNamesJavascript::saveKeyName(QString address, QString name) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsSaveKeyNameResultJs";

    LOG << "save key name " << address << " " << name;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address));

    wrapOperation([&, this](){
        emit manager.saveWalletName(address, name, WalletNames::SaveWalletNameCallback([address, makeFunc] {
            makeFunc.func(TypedException(), address);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletNamesJavascript::getKeyName(QString address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsGetKeyNameResultJs";

    LOG << "get key name " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit manager.getWalletName(address, WalletNames::GetWalletNameCallback([address, makeFunc] (const QString &name) {
            makeFunc.func(TypedException(), address, name);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletNamesJavascript::getAllWalletsInCurrency(QString currency) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsGetAllWalletsInCurrencyResultJs";

    LOG << "get wallets in currency " << currency;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit manager.getAllWalletsCurrency(currency, WalletNames::GetAllWalletsCurrencyCallback([currency, makeFunc] (const std::vector<WalletInfo> &thisWallets, const std::vector<WalletInfo> &otherWallets) {
            makeFunc.func(TypedException(), currency, walletsToJson(thisWallets, otherWallets));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletNamesJavascript::advanceFill(QString jsonNames) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsAdvanceFillResultJs";

    LOG << "Advance fill " << jsonNames;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        const std::vector<WalletInfo> wallets = parseWalletNames(jsonNames);

        emit manager.addOrUpdateWallets(wallets, WalletNames::AddWalletsNamesCallback([makeFunc] {
            makeFunc.func(TypedException(), "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void WalletNamesJavascript::onUpdatedWalletName(const QString &address, const QString &name) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsUpdatedWalletNameResultJs";

    LOG << "updated wallet " << address << " " << name;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, name);
END_SLOT_WRAPPER
}

void WalletNamesJavascript::onWalletsFlushed() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsWalletsFlushedResultJs";

    LOG << "wallets flushed ";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException());
END_SLOT_WRAPPER
}

} // namespace wallet_names
