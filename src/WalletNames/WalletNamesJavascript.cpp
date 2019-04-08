#include "WalletNamesJavascript.h"

#include "Log.h"
#include "check.h"
#include "SlotWrapper.h"
#include "QRegister.h"
#include "makeJsFunc.h"

#include "WalletNames.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

using namespace std::placeholders;

SET_LOG_NAMESPACE("WNS");

namespace wallet_names {

WalletNamesJavascript::WalletNamesJavascript(WalletNames &walletNames, QObject *parent)
    : QObject(parent)
    , manager(walletNames)
{
    CHECK(connect(this, &WalletNamesJavascript::callbackCall, this, &WalletNamesJavascript::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(&manager, &WalletNames::updatedWalletName, this, &WalletNamesJavascript::onUpdatedWalletName), "not connect onUpdatedWalletName");
    CHECK(connect(&manager, &WalletNames::walletsFlushed, this, &WalletNamesJavascript::onWalletsFlushed), "not connect onWalletsFlushed");

    Q_REG(WalletNamesJavascript::Callback, "WalletNamesJavascript::Callback");

    signalFunc = std::bind(&WalletNamesJavascript::callbackCall, this, _1);
}

template<typename... Args>
void WalletNamesJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void WalletNamesJavascript::runJs(const QString &script) {
    LOG << "Javascript " << script;
    emit jsRunSig(script);
}

void WalletNamesJavascript::onCallbackCall(const std::function<void()> &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

static QJsonDocument walletsToJson(const std::vector<WalletInfo> &thisWallets, const std::vector<WalletInfo> &otherWallets) {
    const auto walletsToJson = [](const std::vector<WalletInfo> &wallets) {
        QJsonArray walletsJson;
        for (const WalletInfo &wallet: wallets) {
            QJsonObject walletJson;
            walletJson.insert("address", wallet.address);
            walletJson.insert("name", wallet.name);

            QJsonArray infosJosn;
            for (const WalletInfo::Info &info: wallet.infos) {
                QJsonObject infoJson;
                infoJson.insert("currency", info.currency);
                infoJson.insert("device", info.device);
                infoJson.insert("user", info.user);

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

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address);
    };

    const auto errorFunc = [address, makeFunc](const TypedException &exception) {
        makeFunc(exception, address);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit manager.saveWalletName(address, name, WalletNames::SaveWalletNameCallback([address, makeFunc, errorFunc] {
            makeFunc(TypedException(), address);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void WalletNamesJavascript::getKeyName(QString address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsGetKeyNameResultJs";

    LOG << "get key name " << address;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &name) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, name);
    };

    const auto errorFunc = [address, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, "");
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit manager.getWalletName(address, WalletNames::GetWalletNameCallback([address, makeFunc, errorFunc] (const QString &name) {
            makeFunc(TypedException(), address, name);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void WalletNamesJavascript::getAllWalletsInCurrency(QString currency) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsGetAllWalletsInCurrencyResultJs";

    LOG << "get wallets in currency " << currency;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, currency, result);
    };

    const auto errorFunc = [currency, makeFunc](const TypedException &exception) {
        makeFunc(exception, currency, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit manager.getAllWalletsCurrency(currency, WalletNames::GetAllWalletsCurrencyCallback([currency, makeFunc, errorFunc] (const std::vector<WalletInfo> &thisWallets, const std::vector<WalletInfo> &otherWallets) {
            makeFunc(TypedException(), currency, walletsToJson(thisWallets, otherWallets));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
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
