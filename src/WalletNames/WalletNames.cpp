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

const static QString WALLET_TYPE_TMH = "tmh";
const static QString WALLET_TYPE_MTH = "mth";
const static QString WALLET_TYPE_BTC = "btc";
const static QString WALLET_TYPE_ETH = "eth";

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

    CHECK(connect(this, &WalletNames::callbackCall, this, &WalletNames::onCallbackCall), "not connect onCallbackCall");

    CHECK(connect(&client, &WebSocketClient::messageReceived, this, &WalletNames::onWssMessageReceived), "not connect wssClient");
    CHECK(connect(&authManager, &auth::Auth::logined, this, &WalletNames::onLogined), "not connect onLogined");

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

    emit authManager.reEmit();

    httpClient.setParent(this);
    CHECK(connect(this, &WalletNames::callbackCall, this, &WalletNames::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(&httpClient, &SimpleClient::callbackCall, this, &WalletNames::callbackCall), "not connect callbackCall");
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
    getAllWallets();
    getAllWalletsApps();
}

void WalletNames::timerMethod() {
    getAllWallets();
    getAllWalletsApps();
}

void WalletNames::finishMethod() {
    // empty
}

void WalletNames::getAllWallets() {
    const QString message = makeGetWalletsMessage(id.get(), token, hwid);
    stateRequest = StateRequest::Requested;
    emit client.sendMessage(message);
}

void WalletNames::getAllWalletsApps() {
    LOG << "Sync wallets";
    const auto callbackAppVersion = [this](const std::string &result, const SimpleClient::ServerException &exception) {
        CHECK(!exception.isSet(), "Server error: " + exception.toString());
        QStringList tmhs;
        QStringList mhcs;
        parseAddressListResponse(QString::fromStdString(result), tmhs, mhcs);

        emit javascriptWrapper.createWatchWalletsList(QStringLiteral(""), tmhs);
        emit javascriptWrapper.createWatchWalletsListMHC(QStringLiteral(""), mhcs);
    };

    const QString req = makeGetWalletsAppsMessage(id.get(), token, hwid);
    httpClient.sendMessagePost(
        QUrl(serverName),
        req,
        callbackAppVersion, timeout
    );
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
        const auto processWallets = [this](const JavascriptWrapper::WalletType &type, const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses) {
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

            return std::make_pair(thisWallets, otherWallets);
        };

        if (currency != "all") {
            const JavascriptWrapper::WalletType type = strToWalletType(currency);
            emit javascriptWrapper.getListWallets(type, JavascriptWrapper::WalletsListCallback([type, callback, processWallets](const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses) {
                const auto pair = processWallets(type, hwid, userName, walletAddresses);

                callback.emitFunc(TypedException(), pair.first, pair.second);
            }, callback, signalFunc));
        } else {
            const std::vector<JavascriptWrapper::WalletType> types = {JavascriptWrapper::WalletType::Tmh, JavascriptWrapper::WalletType::Mth, JavascriptWrapper::WalletType::Btc, JavascriptWrapper::WalletType::Eth};
            std::vector<std::vector<QString>> result;
            emit javascriptWrapper.getListWallets(types[0], JavascriptWrapper::WalletsListCallback([this, types, callback, processWallets, result](const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses) mutable {
                result.emplace_back(walletAddresses);
                emit javascriptWrapper.getListWallets(types[1], JavascriptWrapper::WalletsListCallback([this, types, callback, processWallets, result](const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses) mutable {
                    result.emplace_back(walletAddresses);
                    emit javascriptWrapper.getListWallets(types[2], JavascriptWrapper::WalletsListCallback([this, types, callback, processWallets, result](const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses) mutable {
                        result.emplace_back(walletAddresses);
                        emit javascriptWrapper.getListWallets(types[3], JavascriptWrapper::WalletsListCallback([this, types, callback, processWallets, result](const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses) mutable {
                            result.emplace_back(walletAddresses);

                            std::vector<WalletInfo> allOtherWallets;
                            std::vector<WalletInfo> allThisWallets;

                            CHECK(result.size() == types.size(), "Incorrect result");
                            for (size_t i = 0; i < types.size(); i++) {
                                const JavascriptWrapper::WalletType type = types[i];
                                const std::vector<QString> &wallets = result[i];

                                const auto pair = processWallets(type, hwid, userName, wallets);

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
    const auto processWallets = [this](const JavascriptWrapper::WalletType &type, const QString &hwid, const QString &userName, const std::vector<QString> &walletAddresses) {
        const QString typeStr = walletTypeToStr(type);

        std::vector<WalletInfo> thisWallets;
        thisWallets.reserve(walletAddresses.size());
        for (const QString &address: walletAddresses) {
            WalletInfo info = db.getWalletInfo(address);
            info.address = address; // На случай, если вернулся пустой результат
            info.infos.emplace_back(userName, hwid, typeStr);
            thisWallets.emplace_back(info);
        }

        return thisWallets;
    };

    const std::vector<JavascriptWrapper::WalletType> types = {JavascriptWrapper::WalletType::Tmh, JavascriptWrapper::WalletType::Mth, JavascriptWrapper::WalletType::Btc, JavascriptWrapper::WalletType::Eth};
    for (const JavascriptWrapper::WalletType type: types) {
        emit javascriptWrapper.getListWallets(type, JavascriptWrapper::WalletsListCallback([this, type, processWallets](const QString &hw, const QString &userName, const std::vector<QString> &walletAddresses) mutable {
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
        LOG << "Get wallets ok";
        const std::vector<WalletInfo> wallets = parseGetWalletsMessage(messageJson);
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

            getAllWallets();
            getAllWalletsApps();
        }, [](const TypedException &e) {
            LOG << "Error: " << e.description;
        }, signalFunc));
    }
END_SLOT_WRAPPER
}

} // namespace wallet_names
