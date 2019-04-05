#include "WalletNames.h"

#include <set>

#include "Log.h"
#include "QRegister.h"
#include "SlotWrapper.h"

#include "JavascriptWrapper.h"

#include "WalletNamesDbStorage.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("WNS");

namespace wallet_names {

const static QString WALLET_TYPE_TMH = "tmh";
const static QString WALLET_TYPE_MTH = "mth";
const static QString WALLET_TYPE_BTC = "btc";
const static QString WALLET_TYPE_ETH = "eth";

WalletNames::WalletNames(WalletNamesDbStorage &db, JavascriptWrapper &javascriptWrapper)
    : TimerClass(5min, nullptr)
    , db(db)
    , javascriptWrapper(javascriptWrapper)
{
    CHECK(connect(this, &WalletNames::callbackCall, this, &WalletNames::onCallbackCall), "not connect onCallbackCall");

    CHECK(connect(this, &WalletNames::timerEvent, this, &WalletNames::onTimerEvent), "not connect onTimerEvent");
    CHECK(connect(this, &WalletNames::startedEvent, this, &WalletNames::onRun), "not connect run");

    CHECK(connect(this, &WalletNames::addOrUpdateWallets, this, &WalletNames::onAddOrUpdateWallets), "not connect onAddOrUpdateWallets");
    CHECK(connect(this, &WalletNames::saveWalletName, this, &WalletNames::onSaveWalletName), "not connect onSaveWalletName");
    CHECK(connect(this, &WalletNames::getWalletName, this, &WalletNames::onGetWalletName), "not connect onGetWalletName");
    CHECK(connect(this, &WalletNames::getAllWalletsCurrency, this, &WalletNames::onGetAllWalletsCurrency), "not connect onGetAllWalletsCurrency");

    Q_REG(WalletNames::Callback, "WalletNames::Callback");
    Q_REG(AddWalletsNamesCallback, "AddWalletsNamesCallback");
    Q_REG(SaveWalletNameCallback, "SaveWalletNameCallback");
    Q_REG(GetWalletNameCallback, "GetWalletNameCallback");
    Q_REG(GetAllWalletsCurrencyCallback, "GetAllWalletsCurrencyCallback");

    signalFunc = std::bind(&WalletNames::callbackCall, this, _1);

    moveToThread(&thread1); // TODO вызывать в TimerClass
}

void WalletNames::onCallbackCall(WalletNames::Callback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void WalletNames::onRun() {
BEGIN_SLOT_WRAPPER
END_SLOT_WRAPPER
}

void WalletNames::onTimerEvent() {
BEGIN_SLOT_WRAPPER
END_SLOT_WRAPPER
}

void WalletNames::onAddOrUpdateWallets(const std::vector<WalletInfo> &infos, const AddWalletsNamesCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException &exception = apiVrapper2([&]{
        for (const WalletInfo &info: infos) {
            db.addOrUpdateWallet(info);
        }
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void WalletNames::onSaveWalletName(const QString &address, const QString &name, const SaveWalletNameCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException &exception = apiVrapper2([&]{
        db.giveNameWallet(address, name);
    });
    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void WalletNames::onGetWalletName(const QString &address, const GetWalletNameCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString result;
    const TypedException &exception = apiVrapper2([&]{
        result = db.getNameWallet(address);
    });
    callback.emitFunc(exception, result);
END_SLOT_WRAPPER
}

static JavascriptWrapper::WalletType strToWalletType(const QString &currency) {
    if (currency == WALLET_TYPE_TMH) {
        return JavascriptWrapper::WalletType::Tmh;
    } else if (currency == WALLET_TYPE_MTH) {
        return JavascriptWrapper::WalletType::Mth;
    } else if (currency == WALLET_TYPE_BTC) {
        return JavascriptWrapper::WalletType::Btc;
    } else if (currency == WALLET_TYPE_ETH) {
        return JavascriptWrapper::WalletType::Eth;
    } else {
        throwErr(("Incorrect type: " + currency).toStdString());
    }
}

static QString walletTypeToStr(const JavascriptWrapper::WalletType &type) {
    if (type == JavascriptWrapper::WalletType::Tmh) {
        return WALLET_TYPE_TMH;
    } else if (type == JavascriptWrapper::WalletType::Mth) {
        return WALLET_TYPE_MTH;
    } else if (type == JavascriptWrapper::WalletType::Btc) {
        return WALLET_TYPE_BTC;
    } else if (type == JavascriptWrapper::WalletType::Eth) {
        return WALLET_TYPE_ETH;
    } else {
        throwErr("Incorrect type");
    }
}

void WalletNames::onGetAllWalletsCurrency(const QString &currency, const GetAllWalletsCurrencyCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException &exception = apiVrapper2([&]{
        if (currency != "all") {
            const JavascriptWrapper::WalletType type = strToWalletType(currency);
            emit javascriptWrapper.getListWallets(type, JavascriptWrapper::WalletsListCallback([this, type, callback](const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses) {
                const QString typeStr = walletTypeToStr(type);

                std::vector<WalletInfo> otherWallets = db.getWalletsCurrency(typeStr, userName);
                const std::set<QString> walletAddressesSet(walletAddresses.begin(), walletAddresses.end());
                otherWallets.erase(std::remove_if(otherWallets.begin(), otherWallets.end(), [&walletAddressesSet](const WalletInfo &info) {
                    return walletAddressesSet.find(info.address) != walletAddressesSet.end();
                }), otherWallets.end());

                std::vector<WalletInfo> thisWallets;
                thisWallets.reserve(walletAddresses.size());
                for (const QString &address: walletAddresses) {
                    WalletInfo info = db.getWalletInfo(address);
                    info.address = address; // На случай, если вернулся пустой результат
                    info.infos.emplace_back(userName, hwid, typeStr);
                    thisWallets.emplace_back(info);
                }

                callback.emitFunc(TypedException(), thisWallets, otherWallets);
            }, callback, signalFunc));
        }
    });
    if (exception.isSet()) {
        callback.emitException(exception);
    }
END_SLOT_WRAPPER
}

} // namespace wallet_names
