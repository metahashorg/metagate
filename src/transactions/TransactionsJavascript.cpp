#include "TransactionsJavascript.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "makeJsFunc.h"
#include "SlotWrapper.h"

#include "Transactions.h"

namespace transactions {

TransactionsJavascript::TransactionsJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &TransactionsJavascript::callbackCall, this, &TransactionsJavascript::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &TransactionsJavascript::newBalanceSig, this, &TransactionsJavascript::onNewBalance), "not connect onNewBalance");
}

void TransactionsJavascript::onCallbackCall(const std::function<void()> &callback) {
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

static QJsonDocument balanceToJson(const BalanceResponse &balance) {
    QJsonObject messagesBalanceJson;
    messagesBalanceJson.insert("received", balance.received);
    messagesBalanceJson.insert("spent", balance.spent);
    messagesBalanceJson.insert("countReceived", QString::fromStdString(std::to_string(balance.countReceived)));
    messagesBalanceJson.insert("countSpent", QString::fromStdString(std::to_string(balance.countSpent)));
    messagesBalanceJson.insert("currBlock", QString::fromStdString(std::to_string(balance.currBlockNum)));

    return QJsonDocument(messagesBalanceJson);
}

static QJsonDocument txsToJson(const std::vector<Transaction> &txs) {
    QJsonArray messagesTxsJson;
    for (const Transaction &tx: txs) {
        QJsonObject txJson;
        txJson.insert("id", tx.tx);
        txJson.insert("from", tx.from);
        txJson.insert("to", tx.to);
        txJson.insert("value", tx.value);
        txJson.insert("data", tx.data);
        txJson.insert("timestamp", QString::fromStdString(std::to_string(tx.timestamp)));
        txJson.insert("fee", QString::fromStdString(std::to_string(tx.fee)));
        txJson.insert("nonce", QString::fromStdString(std::to_string(tx.nonce)));
        txJson.insert("isInput", tx.isInput);

        messagesTxsJson.push_back(txJson);
    }

    return QJsonDocument(messagesTxsJson);
}

void TransactionsJavascript::onNewBalance(const QString &address, const QString &currency, const BalanceResponse &balance) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "txsNewBalanceJs";

    LOG << "New balance " << address << " " << currency << " " << balance.countReceived << " " << balance.countSpent;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, currency, balanceToJson(balance));
END_SLOT_WRAPPER
}

void TransactionsJavascript::registerAddress(QString address, QString currency, QString type, QString group) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsRegisterAddressJs";

    LOG << "register address " << address << " " << currency << " " << type;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->registerAddress(currency, address, type, group, [this, address, currency, makeFunc](const TypedException &exception) {
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
    std::vector<Transactions::AddressInfo> infos;
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(addressesJson.toUtf8());
    CHECK(jsonResponse.isArray(), "Incorrect json ");
    const QJsonArray &jsonArr = jsonResponse.array();
    for (const QJsonValue &jsonVal: jsonArr) {
        CHECK(jsonVal.isObject(), "Incorrect json");
        const QJsonObject &addressJson = jsonVal.toObject();

        Transactions::AddressInfo addressInfo;
        CHECK(addressJson.contains("address") && addressJson.value("address").isString(), "Incorrect json: address field not found");
        addressInfo.address = addressJson.value("address").toString();
        CHECK(addressJson.contains("currency") && addressJson.value("currency").isString(), "Incorrect json: currency field not found");
        addressInfo.currency = addressJson.value("currency").toString();
        CHECK(addressJson.contains("type") && addressJson.value("type").isString(), "Incorrect json: type field not found");
        addressInfo.type = addressJson.value("type").toString();
        CHECK(addressJson.contains("group") && addressJson.value("group").isString(), "Incorrect json: group field not found");
        addressInfo.group = addressJson.value("group").toString();

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

void TransactionsJavascript::calcBalance(QString address, QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsCalcBalanceResultJs";

    LOG << "calc balance address " << currency << " " << address;

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->calcBalance(address, currency, [this, currency, address, makeFunc](const BalanceResponse &balance, const TypedException &exception) {
            LOG << "Get balance ok " << currency << " " << address;
            makeFunc(exception, address, currency, balanceToJson(balance));
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, address, currency, QJsonDocument());
    }
END_SLOT_WRAPPER
}

}
