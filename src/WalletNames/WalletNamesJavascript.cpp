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

void WalletNamesJavascript::advanceFill(QString jsonNames) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "wnsAdvanceFillResultJs";

    LOG << "Advance fill " << jsonNames;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const auto errorFunc = [makeFunc](const TypedException &exception) {
        makeFunc(exception, "Not ok");
    };

    const TypedException exception = apiVrapper2([&, this](){
        const std::vector<WalletInfo> wallets = parseWalletNames(jsonNames);

        emit manager.addOrUpdateWallets(wallets, WalletNames::AddWalletsNamesCallback([makeFunc] {
            makeFunc(TypedException(), "Ok");
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
