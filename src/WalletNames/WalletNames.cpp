#include "WalletNames.h"

#include <set>

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSettings>

#include "Log.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/SlotWrapper.h"
#include "Paths.h"

#include "auth/Auth.h"

#include "utilites/machine_uid.h"

#include "uploader.h"
#include "utilites/platform.h"
#include "Wallets/Wallet.h"
#include "WalletNamesDbStorage.h"
#include "WalletNamesMessages.h"
#include "qt_utilites/ManagerWrapperImpl.h"

#include "Wallets/WalletInfo.h"
#include "Wallets/Wallets.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("WNS");

namespace wallet_names {

const QString WALLET_CURRENCY_TMH = "tmh";
const QString WALLET_CURRENCY_MTH = "mth";
const QString WALLET_CURRENCY_BTC = "btc";
const QString WALLET_CURRENCY_ETH = "eth";

WalletNames::WalletNames(WalletNamesDbStorage &db, auth::Auth &authManager, WebSocketClient &client, wallets::Wallets &wallets)
    : TimerClass(5min, nullptr)
    , db(db)
    , client(client)
    , wallets(wallets)
{
    serverName = Uploader::getServerName();

    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("timeouts_sec/uploader"), "settings timeouts not found");
    timeout = seconds(settings.value("timeouts_sec/uploader").toInt());

    Q_CONNECT(&client, &WebSocketClient::messageReceived, this, &WalletNames::onWssMessageReceived);
    Q_CONNECT(&authManager, &auth::Auth::logined2, this, &WalletNames::onLogined);

    Q_CONNECT(this, &WalletNames::addOrUpdateWallets, this, &WalletNames::onAddOrUpdateWallets);
    Q_CONNECT(this, &WalletNames::saveWalletName, this, &WalletNames::onSaveWalletName);
    Q_CONNECT(this, &WalletNames::getWalletName, this, &WalletNames::onGetWalletName);
    Q_CONNECT(this, &WalletNames::getAllWalletsCurrency, this, &WalletNames::onGetAllWalletsCurrency);

    Q_CONNECT(&wallets, &wallets::Wallets::mhcWatchWalletCreated, this, &WalletNames::onMhcWatchWalletCreated);
    Q_CONNECT(&wallets, &wallets::Wallets::mhcWatchWalletRemoved, this, &WalletNames::onMhcWatchWalletRemoved);

    Q_REG(WalletNames::Callback, "WalletNames::Callback");
    Q_REG(AddWalletsNamesCallback, "AddWalletsNamesCallback");
    Q_REG(SaveWalletNameCallback, "SaveWalletNameCallback");
    Q_REG(GetWalletNameCallback, "GetWalletNameCallback");
    Q_REG(GetAllWalletsCurrencyCallback, "GetAllWalletsCurrencyCallback");

    emit authManager.reEmit();

    httpClient.setParent(this);
    Q_CONNECT(&httpClient, &SimpleClient::callbackCall, this, &WalletNames::callbackCall);
    httpClient.moveToThread(TimerClass::getThread());

    hwid = QString::fromStdString(getMachineUid());

    moveToThread(TimerClass::getThread()); // TODO вызывать в TimerClass
}

WalletNames::~WalletNames() {
    TimerClass::exit();
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
    const auto callbackAppVersion = [this](const SimpleClient::Response &response) {
        CHECK(!response.exception.isSet(), "Server error: " + response.exception.toString());
        const std::vector<WalletInfo> wallets = parseAddressListResponse(QString::fromStdString(response.response));
        std::vector<QString> tmhs;
        std::vector<QString> mhcs;
        for (const WalletInfo &wallet: wallets) {
            if (wallet.currentInfo.type != WalletInfo::Info::Type::Watch) {
                continue;
            }
            if (wallet.currentInfo.currency == WALLET_CURRENCY_TMH) {
                tmhs.emplace_back(wallet.address);
            } else if (wallet.currentInfo.currency == WALLET_CURRENCY_MTH) {
                mhcs.emplace_back(wallet.address);
            }
        }

        emit this->wallets.createWatchWalletsList(false, tmhs, wallets::Wallets::CreateWatchsCallback([](const std::vector<std::pair<QString, QString>> &){}, [](const TypedException &e) {
            LOG << "Error while create watch wallets: " << e.description;
        }, signalFunc));
        emit this->wallets.createWatchWalletsList(true, mhcs, wallets::Wallets::CreateWatchsCallback([](const std::vector<std::pair<QString, QString>> &){}, [](const TypedException &e) {
            LOG << "Error while create watch wallets: " << e.description;
        }, signalFunc));

        processWalletsList(wallets);
    };

    stateRequest = StateRequest::Requested;

    const QString req = makeGetWalletsAppsMessage(id.get(), token, hwid);
    httpClient.sendMessagePost(QUrl(serverName), req, callbackAppVersion, timeout);
}

void WalletNames::onAddOrUpdateWallets(const std::vector<WalletInfo> &infos, const AddWalletsNamesCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        for (const WalletInfo &info: infos) {
            db.addOrUpdateWallet(info);
        }
    }, callback);
END_SLOT_WRAPPER
}

void WalletNames::onSaveWalletName(const QString &address, const QString &name, const SaveWalletNameCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        db.giveNameWallet(address, name);
        const QString message = makeRenameMessage(address, name, id.get(), token, hwid);
        if (stateRequest == StateRequest::Requested) {
            stateRequest = StateRequest::Intercepted;
        }
        emit client.sendMessage(message);

        const QString message2 = makeRenameMessageHttp(address, name, WALLET_CURRENCY_MTH, id.get(), token, hwid);
        emit httpClient.sendMessagePost(serverName, message2, [](const SimpleClient::Response &response) {
            CHECK(!response.exception.isSet(), "Server error: " + response.exception.toString());
        });
    }, callback);
END_SLOT_WRAPPER
}

void WalletNames::onGetWalletName(const QString &address, const GetWalletNameCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]{
        return db.getNameWallet(address);
    }, callback);
END_SLOT_WRAPPER
}

static wallets::WalletCurrency strToWalletCurrency(const QString &currency) {
    if (currency == WALLET_CURRENCY_TMH) {
        return wallets::WalletCurrency::Tmh;
    } else if (currency == WALLET_CURRENCY_MTH) {
        return wallets::WalletCurrency::Mth;
    } else if (currency == WALLET_CURRENCY_BTC) {
        return wallets::WalletCurrency::Btc;
    } else if (currency == WALLET_CURRENCY_ETH) {
        return wallets::WalletCurrency::Eth;
    } else {
        throwErr(("Incorrect type: " + currency).toStdString());
    }
}

static QString walletCurrencyToStr(const wallets::WalletCurrency &type) {
    if (type == wallets::WalletCurrency::Tmh) {
        return WALLET_CURRENCY_TMH;
    } else if (type == wallets::WalletCurrency::Mth) {
        return WALLET_CURRENCY_MTH;
    } else if (type == wallets::WalletCurrency::Btc) {
        return WALLET_CURRENCY_BTC;
    } else if (type == wallets::WalletCurrency::Eth) {
        return WALLET_CURRENCY_ETH;
    } else {
        throwErr("Incorrect type");
    }
}

static WalletInfo::Info::Type convertTypes(const wallets::WalletInfo::Type &type) {
    if (type == wallets::WalletInfo::Type::Key) {
        return WalletInfo::Info::Type::Key;
    } else if (type == wallets::WalletInfo::Type::Watch) {
        return WalletInfo::Info::Type::Watch;
    } else {
        throwErr("Unknown type");
    }
}

void WalletNames::onGetAllWalletsCurrency(const QString &currency, const GetAllWalletsCurrencyCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitErrorCallback([&]{
        const auto processWallets = [this](const wallets::WalletCurrency &currency, QString userName, const std::vector<wallets::WalletInfo> &walletAddresses) {
            const QString currencyStr = walletCurrencyToStr(currency);

            if (userName.isEmpty()) {
                userName = "_unregistered";
            }

            std::vector<WalletInfo> otherWallets = db.getWalletsCurrency(currencyStr, userName);
            std::set<QString> walletAddressesSet;
            std::transform(walletAddresses.begin(), walletAddresses.end(), std::inserter(walletAddressesSet, walletAddressesSet.begin()), std::mem_fn(&wallets::WalletInfo::address));
            otherWallets.erase(std::remove_if(otherWallets.begin(), otherWallets.end(), [&walletAddressesSet](const WalletInfo &info) {
                return walletAddressesSet.find(info.address) != walletAddressesSet.end();
            }), otherWallets.end());

            std::vector<WalletInfo> thisWallets;
            thisWallets.reserve(walletAddresses.size());
            for (const wallets::WalletInfo &wallet: walletAddresses) {
                WalletInfo info = db.getWalletInfo(wallet.address);
                info.address = wallet.address; // На случай, если вернулся пустой результат
                info.infos.emplace_back(userName, hwid, currencyStr, convertTypes(wallet.type));
                info.currentInfo = info.infos.back();
                thisWallets.emplace_back(info);
            }

            return std::make_pair(thisWallets, otherWallets);
        };

        if (currency != "all") {
            const wallets::WalletCurrency c = strToWalletCurrency(currency);
            emit wallets.getListWallets(c, wallets::Wallets::WalletsListCallback([c, callback, processWallets](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddresses) {
                const auto pair = processWallets(c, userName, walletAddresses);

                callback.emitFunc(TypedException(), pair.first, pair.second);
            }, callback, signalFunc));
        } else {
            const std::vector<wallets::WalletCurrency> currencys = {wallets::WalletCurrency::Tmh, wallets::WalletCurrency::Mth, wallets::WalletCurrency::Btc, wallets::WalletCurrency::Eth};
            std::vector<std::vector<wallets::WalletInfo>> result;
            emit wallets.getListWallets(currencys[0], wallets::Wallets::WalletsListCallback([this, currencys, callback, processWallets, result](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddresses) mutable {
                result.emplace_back(walletAddresses);
                emit wallets.getListWallets(currencys[1], wallets::Wallets::WalletsListCallback([this, currencys, callback, processWallets, result, u=userName](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddresses) mutable {
                    CHECK(u == userName, "Username changed while get wallets");
                    result.emplace_back(walletAddresses);
                    emit wallets.getListWallets(currencys[2], wallets::Wallets::WalletsListCallback([this, currencys, callback, processWallets, result, u=userName](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddresses) mutable {
                        CHECK(u == userName, "Username changed while get wallets");
                        result.emplace_back(walletAddresses);
                        emit wallets.getListWallets(currencys[3], wallets::Wallets::WalletsListCallback([currencys, callback, processWallets, result, u=userName](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddresses) mutable {
                            CHECK(u == userName, "Username changed while get wallets");
                            result.emplace_back(walletAddresses);

                            std::vector<WalletInfo> allOtherWallets;
                            std::vector<WalletInfo> allThisWallets;

                            CHECK(result.size() == currencys.size(), "Incorrect result");
                            for (size_t i = 0; i < currencys.size(); i++) {
                                const wallets::WalletCurrency currency = currencys[i];
                                const std::vector<wallets::WalletInfo> &wallets = result[i];

                                const auto pair = processWallets(currency, userName, wallets);

                                allOtherWallets.insert(allOtherWallets.end(), pair.second.begin(), pair.second.end());
                                allThisWallets.insert(allThisWallets.end(), pair.first.begin(), pair.first.end());
                            }

                            callback.emitFunc(TypedException(), allThisWallets, allOtherWallets);
                        }, callback, signalFunc));
                    }, callback, signalFunc));
                }, callback, signalFunc));
            }, callback, signalFunc));
        }
    }, callback);
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
    const auto processWallets = [this](const wallets::WalletCurrency &type, QString userName, const std::vector<wallets::WalletInfo> &walletAddresses) {
        const QString typeStr = walletCurrencyToStr(type);

        std::vector<WalletInfo> thisWallets;
        thisWallets.reserve(walletAddresses.size());
        for (const wallets::WalletInfo &wallet: walletAddresses) {
            WalletInfo info = db.getWalletInfo(wallet.address);
            info.address = wallet.address; // На случай, если вернулся пустой результат
            if (userName.isEmpty()) {
                userName = "_unregistered";
            }
            info.infos.emplace_back(userName, hwid, typeStr, convertTypes(wallet.type));
            thisWallets.emplace_back(info);
        }

        return thisWallets;
    };

    LOG << "Send all wallets";

    const std::vector<wallets::WalletCurrency> types = {wallets::WalletCurrency::Tmh, wallets::WalletCurrency::Mth, wallets::WalletCurrency::Btc, wallets::WalletCurrency::Eth};
    for (const wallets::WalletCurrency type: types) {
        emit wallets.getListWallets(type, wallets::Wallets::WalletsListCallback([this, type, processWallets](const QString &userName, const std::vector<wallets::WalletInfo> &walletAddresses) mutable {
            const std::vector<WalletInfo> wallets = processWallets(type, userName, walletAddresses);
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

void WalletNames::onLogined(bool isInit, const QString &login, const QString &token_) {
BEGIN_SLOT_WRAPPER
    userName = login;
    token = token_;

    if (isInit) {
        getAllWalletsApps();
    }
END_SLOT_WRAPPER
}

void WalletNames::onMhcWatchWalletCreated(bool isMhc, const QString &address, const QString &username) {
BEGIN_SLOT_WRAPPER
    if (userName != username) {
        return;
    }
    const QString message = makeCreateWatchWalletMessage(id.get(), token, hwid, address, isMhc);

    httpClient.sendMessagePost(serverName, message, SimpleClient::ClientCallback([](const SimpleClient::Response &response) {
        CHECK(!response.exception.isSet(), response.exception.toString());
    }));
END_SLOT_WRAPPER
}

void WalletNames::onMhcWatchWalletRemoved(bool isMhc, const QString &address, const QString &username) {
BEGIN_SLOT_WRAPPER
    if (userName != username) {
        return;
    }
    const QString message = makeRemoveWatchWalletMessage(id.get(), token, hwid, address, isMhc);

    httpClient.sendMessagePost(serverName, message, SimpleClient::ClientCallback([](const SimpleClient::Response &response) {
        CHECK(!response.exception.isSet(), response.exception.toString());
    }));
END_SLOT_WRAPPER
}

} // namespace wallet_names
