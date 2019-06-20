#include "WalletNames.h"

#include <set>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>

#include "Log.h"
#include "QRegister.h"
#include "SlotWrapper.h"
#include "Paths.h"

#include "JavascriptWrapper.h"

#include "auth/Auth.h"

#include "machine_uid.h"

#include "uploader.h"
#include "platform.h"
#include "Wallet.h"
#include "WalletNamesDbStorage.h"
#include "WalletNamesMessages.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("WNS");

namespace wallet_names {

const QString WALLET_CURRENCY_TMH = "tmh";
const QString WALLET_CURRENCY_MTH = "mth";
const QString WALLET_CURRENCY_BTC = "btc";
const QString WALLET_CURRENCY_ETH = "eth";

WalletNames::WalletNames(WalletNamesDbStorage &db, JavascriptWrapper &javascriptWrapper, auth::Auth &authManager, WebSocketClient &client)
    : TimerClass(5min, nullptr)
    , db(db)
    , javascriptWrapper(javascriptWrapper)
    , authManager(authManager)
    , client(client)
{
    serverName = Uploader::getServerName();

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("timeouts_sec/uploader"), "settings timeouts not found");
    timeout = seconds(settings.value("timeouts_sec/uploader").toInt());

    Q_CONNECT(this, &WalletNames::callbackCall, this, &WalletNames::onCallbackCall);

    Q_CONNECT(&client, &WebSocketClient::messageReceived, this, &WalletNames::onWssMessageReceived);
    Q_CONNECT(&authManager, &auth::Auth::logined, this, &WalletNames::onLogined);

    Q_CONNECT(this, &WalletNames::addOrUpdateWallets, this, &WalletNames::onAddOrUpdateWallets);
    Q_CONNECT(this, &WalletNames::saveWalletName, this, &WalletNames::onSaveWalletName);
    Q_CONNECT(this, &WalletNames::getWalletName, this, &WalletNames::onGetWalletName);
    Q_CONNECT(this, &WalletNames::getAllWalletsCurrency, this, &WalletNames::onGetAllWalletsCurrency);

    Q_REG(WalletNames::Callback, "WalletNames::Callback");
    Q_REG(AddWalletsNamesCallback, "AddWalletsNamesCallback");
    Q_REG(SaveWalletNameCallback, "SaveWalletNameCallback");
    Q_REG(GetWalletNameCallback, "GetWalletNameCallback");
    Q_REG(GetAllWalletsCurrencyCallback, "GetAllWalletsCurrencyCallback");

    signalFunc = std::bind(&WalletNames::callbackCall, this, _1);

    emit authManager.reEmit();

    httpClient.setParent(this);
    Q_CONNECT(&httpClient, &SimpleClient::callbackCall, this, &WalletNames::callbackCall);
    httpClient.moveToThread(TimerClass::getThread());

    moveToThread(TimerClass::getThread()); // TODO вызывать в TimerClass
}

WalletNames::~WalletNames() {
    TimerClass::exit();
}

void WalletNames::onCallbackCall(WalletNames::Callback callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void WalletNames::startMethod() {
    //getAllWallets();
    getAllWalletsApps();
}

void WalletNames::timerMethod() {
    //getAllWallets();
    getAllWalletsApps();
}

void WalletNames::finishMethod() {
    // empty
}

void WalletNames::getAllWallets() {
    if (token.isEmpty()) {
        return;
    }
    LOG << "Sync wallets2";
    const QString message = makeGetWalletsMessage(id.get(), token, hwid);
    stateRequest = StateRequest::Requested;
    emit client.sendMessage(message);
}

void WalletNames::getAllWalletsApps() {
    if (token.isEmpty()) {
        return;
    }
    LOG << "Sync wallets";
    const auto callbackAppVersion = [this](const std::string &result, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        const std::vector<WalletInfo> wallets = parseAddressListResponse(QString::fromStdString(result));
        QStringList tmhs;
        QStringList mhcs;
        for (const WalletInfo &wallet: wallets) {
            if (wallet.currentInfo.type != WalletInfo::Info::Type::Watch) {
                continue;
            }
            if (wallet.currentInfo.currency == WALLET_CURRENCY_TMH) {
                tmhs.append(wallet.address);
            } else if (wallet.currentInfo.currency == WALLET_CURRENCY_MTH) {
                mhcs.append(wallet.address);
            }
        }

        emit javascriptWrapper.createWatchWalletsList(QStringLiteral(""), tmhs);
        emit javascriptWrapper.createWatchWalletsListMHC(QStringLiteral(""), mhcs);

        processWalletsList(wallets);
    };

    stateRequest = StateRequest::Requested;

    const QString req = makeGetWalletsAppsMessage(id.get(), token, hwid);
    httpClient.sendMessagePost(QUrl(serverName), req, callbackAppVersion, timeout);
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
        const QString message = makeRenameMessage(address, name, id.get(), token, hwid);
        if (stateRequest == StateRequest::Requested) {
            stateRequest = StateRequest::Intercepted;
        }
        emit client.sendMessage(message);

        const QString message2 = makeRenameMessageHttp(address, name, WALLET_CURRENCY_MTH, id.get(), token, hwid);
        emit httpClient.sendMessagePost(serverName, message2, [](const std::string &result, const SimpleClient::ServerException &exception) {
            CHECK(!exception.isSet(), "Server error: " + exception.toString());
        });
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

static JavascriptWrapper::WalletCurrency strToWalletCurrency(const QString &currency) {
    if (currency == WALLET_CURRENCY_TMH) {
        return JavascriptWrapper::WalletCurrency::Tmh;
    } else if (currency == WALLET_CURRENCY_MTH) {
        return JavascriptWrapper::WalletCurrency::Mth;
    } else if (currency == WALLET_CURRENCY_BTC) {
        return JavascriptWrapper::WalletCurrency::Btc;
    } else if (currency == WALLET_CURRENCY_ETH) {
        return JavascriptWrapper::WalletCurrency::Eth;
    } else {
        throwErr(("Incorrect type: " + currency).toStdString());
    }
}

static QString walletCurrencyToStr(const JavascriptWrapper::WalletCurrency &type) {
    if (type == JavascriptWrapper::WalletCurrency::Tmh) {
        return WALLET_CURRENCY_TMH;
    } else if (type == JavascriptWrapper::WalletCurrency::Mth) {
        return WALLET_CURRENCY_MTH;
    } else if (type == JavascriptWrapper::WalletCurrency::Btc) {
        return WALLET_CURRENCY_BTC;
    } else if (type == JavascriptWrapper::WalletCurrency::Eth) {
        return WALLET_CURRENCY_ETH;
    } else {
        throwErr("Incorrect type");
    }
}

static WalletInfo::Info::Type convertTypes(const Wallet::Type &type) {
    if (type == Wallet::Type::Key) {
        return WalletInfo::Info::Type::Key;
    } else if (type == Wallet::Type::Watch) {
        return WalletInfo::Info::Type::Watch;
    } else {
        throwErr("Unknown type");
    }
}

void WalletNames::onGetAllWalletsCurrency(const QString &currency, const GetAllWalletsCurrencyCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException &exception = apiVrapper2([&]{
        const auto processWallets = [this](const JavascriptWrapper::WalletCurrency &currency, const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) {
            const QString currencyStr = walletCurrencyToStr(currency);

            std::vector<WalletInfo> otherWallets = db.getWalletsCurrency(currencyStr, userName);
            std::set<QString> walletAddressesSet;
            std::transform(walletAddresses.begin(), walletAddresses.end(), std::inserter(walletAddressesSet, walletAddressesSet.begin()), std::mem_fn(&Wallet::WalletInfo::address));
            otherWallets.erase(std::remove_if(otherWallets.begin(), otherWallets.end(), [&walletAddressesSet](const WalletInfo &info) {
                return walletAddressesSet.find(info.address) != walletAddressesSet.end();
            }), otherWallets.end());

            std::vector<WalletInfo> thisWallets;
            thisWallets.reserve(walletAddresses.size());
            for (const Wallet::WalletInfo &wallet: walletAddresses) {
                WalletInfo info = db.getWalletInfo(wallet.address);
                info.address = wallet.address; // На случай, если вернулся пустой результат
                info.infos.emplace_back(userName, hwid, currencyStr, convertTypes(wallet.type));
                info.currentInfo = info.infos.back();
                thisWallets.emplace_back(info);
            }

            return std::make_pair(thisWallets, otherWallets);
        };

        if (currency != "all") {
            const JavascriptWrapper::WalletCurrency c = strToWalletCurrency(currency);
            emit javascriptWrapper.getListWallets(c, JavascriptWrapper::WalletsListCallback([c, callback, processWallets](const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) {
                const auto pair = processWallets(c, hwid, userName, walletAddresses);

                callback.emitFunc(TypedException(), pair.first, pair.second);
            }, callback, signalFunc));
        } else {
            const std::vector<JavascriptWrapper::WalletCurrency> currencys = {JavascriptWrapper::WalletCurrency::Tmh, JavascriptWrapper::WalletCurrency::Mth, JavascriptWrapper::WalletCurrency::Btc, JavascriptWrapper::WalletCurrency::Eth};
            std::vector<std::vector<Wallet::WalletInfo>> result;
            emit javascriptWrapper.getListWallets(currencys[0], JavascriptWrapper::WalletsListCallback([this, currencys, callback, processWallets, result](const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) mutable {
                result.emplace_back(walletAddresses);
                emit javascriptWrapper.getListWallets(currencys[1], JavascriptWrapper::WalletsListCallback([this, currencys, callback, processWallets, result](const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) mutable {
                    result.emplace_back(walletAddresses);
                    emit javascriptWrapper.getListWallets(currencys[2], JavascriptWrapper::WalletsListCallback([this, currencys, callback, processWallets, result](const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) mutable {
                        result.emplace_back(walletAddresses);
                        emit javascriptWrapper.getListWallets(currencys[3], JavascriptWrapper::WalletsListCallback([currencys, callback, processWallets, result](const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) mutable {
                            result.emplace_back(walletAddresses);

                            std::vector<WalletInfo> allOtherWallets;
                            std::vector<WalletInfo> allThisWallets;

                            CHECK(result.size() == currencys.size(), "Incorrect result");
                            for (size_t i = 0; i < currencys.size(); i++) {
                                const JavascriptWrapper::WalletCurrency currency = currencys[i];
                                const std::vector<Wallet::WalletInfo> &wallets = result[i];

                                const auto pair = processWallets(currency, hwid, userName, wallets);

                                allOtherWallets.insert(allOtherWallets.end(), pair.second.begin(), pair.second.end());
                                allThisWallets.insert(allThisWallets.end(), pair.first.begin(), pair.first.end());
                            }

                            callback.emitFunc(TypedException(), allThisWallets, allOtherWallets);
                        }, callback, signalFunc));
                    }, callback, signalFunc));
                }, callback, signalFunc));
            }, callback, signalFunc));
        }
    });
    if (exception.isSet()) {
        callback.emitException(exception);
    }
END_SLOT_WRAPPER
}

void WalletNames::processWalletsList(const std::vector<WalletInfo> &wallets) {
    if (stateRequest == StateRequest::Intercepted) {
        return;
    }

    stateRequest = StateRequest::NotRequest;

    for (const WalletInfo &info: wallets) {
        const bool isUpdated = db.addOrUpdateWallet(info);
        if (isUpdated) {
            emit updatedWalletName(info.address, info.name);
        }
    }

    emit walletsFlushed();

    sendAllWallets();
}

void WalletNames::sendAllWallets() {
    const auto processWallets = [this](const JavascriptWrapper::WalletCurrency &type, const QString &hwid, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) {
        const QString typeStr = walletCurrencyToStr(type);

        std::vector<WalletInfo> thisWallets;
        thisWallets.reserve(walletAddresses.size());
        for (const Wallet::WalletInfo &wallet: walletAddresses) {
            WalletInfo info = db.getWalletInfo(wallet.address);
            info.address = wallet.address; // На случай, если вернулся пустой результат
            info.infos.emplace_back(userName, hwid, typeStr, convertTypes(wallet.type));
            thisWallets.emplace_back(info);
        }

        return thisWallets;
    };

    LOG << "Send all wallets";

    const std::vector<JavascriptWrapper::WalletCurrency> types = {JavascriptWrapper::WalletCurrency::Tmh, JavascriptWrapper::WalletCurrency::Mth, JavascriptWrapper::WalletCurrency::Btc, JavascriptWrapper::WalletCurrency::Eth};
    for (const JavascriptWrapper::WalletCurrency type: types) {
        emit javascriptWrapper.getListWallets(type, JavascriptWrapper::WalletsListCallback([this, type, processWallets](const QString &hw, const QString &userName, const std::vector<Wallet::WalletInfo> &walletAddresses) mutable {
            const std::vector<WalletInfo> wallets = processWallets(type, hw, userName, walletAddresses);
            const QString message = makeSetWalletsMessage(wallets, id.get(), token, hwid);
            client.sendMessage(message);
        }, [](const TypedException &e) {
            LOG << "Error: " << e.description;
        }, signalFunc));
    }
}

void WalletNames::onWssMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument messageJson = QJsonDocument::fromJson(message.toUtf8());
    const ResponseType responseType = getMethodAndAddressResponse(messageJson);

    if (responseType.isError) {
        LOG << "WalletNames response error " << responseType.id << " " << responseType.method << " " << responseType.error;
        return;
    }

    if (responseType.method == METHOD::RENAME) {
        LOG << "Rename wallet ok";
    } else if (responseType.method == METHOD::SET_WALLETS) {
        LOG << "Set wallets ok";
    } else if (responseType.method == METHOD::GET_WALLETS) {
        const std::vector<WalletInfo> wallets = parseGetWalletsMessage(messageJson);
        LOG << "Get wallets ok " << wallets.size();
        processWalletsList(wallets);
    } else if (responseType.method == METHOD::ALIEN) {
        return;
    } else {
        throwErr("Incorrect response type");
    }
END_SLOT_WRAPPER
}

void WalletNames::onLogined(bool isInit, const QString login) {
BEGIN_SLOT_WRAPPER
    if (isInit) {
        authManager.getLoginInfo(auth::Auth::LoginInfoCallback([this](const auth::LoginInfo &info) {
            token = info.token;
            hwid = QString::fromStdString(getMachineUid());

            //getAllWallets();
            getAllWalletsApps();
        }, [](const TypedException &e) {
            LOG << "Error: " << e.description;
        }, signalFunc));
    }
END_SLOT_WRAPPER
}

} // namespace wallet_names
