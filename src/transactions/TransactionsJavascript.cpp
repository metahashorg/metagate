#include "TransactionsJavascript.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "Transaction.h"

#include "makeJsFunc.h"
#include "SlotWrapper.h"

#include "Transactions.h"

namespace transactions {

TransactionsJavascript::TransactionsJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &TransactionsJavascript::callbackCall, this, &TransactionsJavascript::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &TransactionsJavascript::newBalanceSig, this, &TransactionsJavascript::onNewBalance), "not connect onNewBalance");
    CHECK(connect(this, &TransactionsJavascript::sendedTransactionsResponseSig, this, &TransactionsJavascript::onSendedTransactionsResponse), "not connect onSendedTransactionsResponse");
    CHECK(connect(this, &TransactionsJavascript::transactionInTorrentSig, this, &TransactionsJavascript::onTransactionInTorrent), "not connect onTransactionInTorrent");

    qRegisterMetaType<Callback>("Callback");

    qRegisterMetaType<BalanceInfo>("BalanceInfo");
    qRegisterMetaType<BalanceInfo>("BalanceInfo");
    qRegisterMetaType<Transaction>("Transaction");
}

void TransactionsJavascript::onCallbackCall(const Callback &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

template<typename... Args>
void TransactionsJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void TransactionsJavascript::runJs(const QString &script) {
    LOG << "Javascript " << script;
    emit jsRunSig(script);
}

static QJsonDocument balanceToJson(const BalanceInfo &balance) {
    QJsonObject messagesBalanceJson;
    messagesBalanceJson.insert("received", balance.received);
    messagesBalanceJson.insert("spent", balance.spent);
    messagesBalanceJson.insert("countReceived", QString::fromStdString(std::to_string(balance.countReceived)));
    messagesBalanceJson.insert("countSpent", QString::fromStdString(std::to_string(balance.countSpent)));
    messagesBalanceJson.insert("currBlock", QString::fromStdString(std::to_string(balance.currBlockNum)));

    return QJsonDocument(messagesBalanceJson);
}

static QJsonObject txToJson(const Transaction &tx) {
    QJsonObject txJson;
    txJson.insert("id", tx.tx);
    txJson.insert("from", tx.from);
    txJson.insert("to", tx.to);
    txJson.insert("value", tx.value);
    txJson.insert("data", tx.data);
    txJson.insert("timestamp", QString::fromStdString(std::to_string(tx.timestamp)));
    txJson.insert("fee", tx.fee);
    txJson.insert("nonce", QString::fromStdString(std::to_string(tx.nonce)));
    txJson.insert("isInput", tx.isInput);
    return txJson;
}

static QJsonDocument txsToJson(const std::vector<Transaction> &txs) {
    QJsonArray messagesTxsJson;
    for (const Transaction &tx: txs) {
        messagesTxsJson.push_back(txToJson(tx));
    }

    return QJsonDocument(messagesTxsJson);
}

static QJsonDocument addressInfoToJson(const std::vector<AddressInfo> &infos) {
    QJsonArray messagesInfosJson;
    for (const AddressInfo &info: infos) {
        QJsonObject txJson;
        txJson.insert("address", info.address);
        txJson.insert("currency", info.currency);
        txJson.insert("group", info.group);
        txJson.insert("type", info.type);
        txJson.insert("name", info.name);

        messagesInfosJson.push_back(txJson);
    }

    return QJsonDocument(messagesInfosJson);
}

static QJsonDocument txInfoToJson(const Transaction &tx) {
    return QJsonDocument(txToJson(tx));
}

void TransactionsJavascript::onNewBalance(const QString &address, const QString &currency, const BalanceInfo &balance) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "txsNewBalanceJs";

    LOG << "New balance " << address << " " << currency << " " << balance.countReceived << " " << balance.countSpent;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, currency, balanceToJson(balance));
END_SLOT_WRAPPER
}

void TransactionsJavascript::registerAddress(QString address, QString currency, QString type, QString group, QString name) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsRegisterAddressJs";

    LOG << "register address " << address << " " << currency << " " << type;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency);
    };

    const TypedException exception = apiVrapper2([&, this](){
        AddressInfo info(currency, address, type, group, name);
        emit transactionsManager->registerAddresses({info}, [this, address, currency, makeFunc](const TypedException &exception) {
            makeFunc(exception, address, currency);
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, address, currency);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::registerAddresses(QString addressesJson) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsRegisterAddressesJs";
    std::vector<AddressInfo> infos;
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(addressesJson.toUtf8());
    CHECK(jsonResponse.isArray(), "Incorrect json ");
    const QJsonArray &jsonArr = jsonResponse.array();
    for (const QJsonValue &jsonVal: jsonArr) {
        CHECK(jsonVal.isObject(), "Incorrect json");
        const QJsonObject &addressJson = jsonVal.toObject();

        AddressInfo addressInfo;
        CHECK(addressJson.contains("address") && addressJson.value("address").isString(), "Incorrect json: address field not found");
        addressInfo.address = addressJson.value("address").toString();
        CHECK(addressJson.contains("currency") && addressJson.value("currency").isString(), "Incorrect json: currency field not found");
        addressInfo.currency = addressJson.value("currency").toString();
        CHECK(addressJson.contains("type") && addressJson.value("type").isString(), "Incorrect json: type field not found");
        addressInfo.type = addressJson.value("type").toString();
        CHECK(addressJson.contains("group") && addressJson.value("group").isString(), "Incorrect json: group field not found");
        addressInfo.group = addressJson.value("group").toString();
        CHECK(addressJson.contains("name") && addressJson.value("name").isString(), "Incorrect json: name field not found");
        addressInfo.name = addressJson.value("name").toString();

        infos.emplace_back(addressInfo);
    }

    LOG << "register addresses " << infos.size();

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, QString("Ok"));
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->registerAddresses(infos, [this, makeFunc](const TypedException &exception) {
            makeFunc(exception);
        });
    });

    if (exception.isSet()) {
        makeFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getAddresses(QString group) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetAddressesResultJs";

    LOG << "get addresses " << group;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getAddresses(group, [this, makeFunc](const std::vector<AddressInfo> &infos, const TypedException &exception) {
            LOG << "Addresses getted " << infos.size();
            const QJsonDocument &result = addressInfoToJson(infos);
            makeFunc(exception, result);
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::setCurrentGroup(QString group) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsSetCurrentGroupResultJs";

    LOG << "Set group " << group;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, QString("Ok"));
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->setCurrentGroup(group, [this, makeFunc](const TypedException &exception) {
            makeFunc(exception);
        });
    });

    if (exception.isSet()) {
        makeFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxs(QString address, QString currency, QString fromTx, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsJs";

    LOG << "get txs address " << address << " " << currency << " " << fromTx;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxs(address, currency, fromTx, count, asc, [this, address, currency, makeFunc](const std::vector<Transaction> &txs, const TypedException &exception) {
            LOG << "Get txs ok " << address << " " << currency << " " << txs.size();
            makeFunc(exception, address, currency, txsToJson(txs));
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, address, currency, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxsAll(QString currency, QString fromTx, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsAllJs";

    LOG << "get txs address " << currency << " " << fromTx;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, currency, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxsAll(currency, fromTx, count, asc, [this, currency, makeFunc](const std::vector<Transaction> &txs, const TypedException &exception) {
            LOG << "Get txs ok " << currency << " " << txs.size();
            makeFunc(exception, currency, txsToJson(txs));
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, currency, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxs2(QString address, QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxs2Js";

    LOG << "get txs2 address " << address << " " << currency << " " << from;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxs2(address, currency, from, count, asc, [this, address, currency, makeFunc](const std::vector<Transaction> &txs, const TypedException &exception) {
            LOG << "Get txs ok " << address << " " << currency << " " << txs.size();
            makeFunc(exception, address, currency, txsToJson(txs));
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, address, currency, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxsAll2(QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsAll2Js";

    LOG << "get txs address " << currency << " " << from;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, currency, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxsAll2(currency, from, count, asc, [this, currency, makeFunc](const std::vector<Transaction> &txs, const TypedException &exception) {
            LOG << "Get txs ok " << currency << " " << txs.size();
            makeFunc(exception, currency, txsToJson(txs));
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, currency, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::calcBalance(QString address, QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsCalcBalanceResultJs";

    LOG << "calc balance address " << currency << " " << address;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->calcBalance(address, currency, [this, currency, address, makeFunc](const BalanceInfo &balance, const TypedException &exception) {
            LOG << "Get balance ok " << currency << " " << address;
            makeFunc(exception, address, currency, balanceToJson(balance));
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, address, currency, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxFromServer(QString txHash, QString type) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxFromServerResultJs";

    LOG << "getTxFromServer address " << txHash << " " << type;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &txHash, const QString &type, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, txHash, type, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxFromServer(txHash, type, [this, txHash, type, makeFunc](const Transaction &tx, const TypedException &exception) {
            LOG << "Get transaction ok " << txHash << " " << type;
            makeFunc(exception, txHash, type, txInfoToJson(tx));
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, txHash, type, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::onSendedTransactionsResponse(const QString &requestId, const QString &server, const QString &response, const TypedException &error) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "txsSendedTxJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, requestId, server, response);
END_SLOT_WRAPPER
}

void TransactionsJavascript::onTransactionInTorrent(const QString &server, const QString &txHash, const Transaction &tx, const TypedException &error) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "txOnTorrentJs";
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, server, txHash, txInfoToJson(tx));
END_SLOT_WRAPPER
}

}
