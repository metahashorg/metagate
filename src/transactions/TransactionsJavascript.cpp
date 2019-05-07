#include "TransactionsJavascript.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "Transaction.h"

#include "makeJsFunc.h"
#include "SlotWrapper.h"
#include "QRegister.h"

#include "Transactions.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("TXS");

namespace transactions {

TransactionsJavascript::TransactionsJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &TransactionsJavascript::callbackCall, this, &TransactionsJavascript::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &TransactionsJavascript::newBalanceSig, this, &TransactionsJavascript::onNewBalance), "not connect onNewBalance");
    CHECK(connect(this, &TransactionsJavascript::sendedTransactionsResponseSig, this, &TransactionsJavascript::onSendedTransactionsResponse), "not connect onSendedTransactionsResponse");
    CHECK(connect(this, &TransactionsJavascript::transactionInTorrentSig, this, &TransactionsJavascript::onTransactionInTorrent), "not connect onTransactionInTorrent");
    CHECK(connect(this, &TransactionsJavascript::transactionStatusChangedSig, this, &TransactionsJavascript::onTransactionStatusChanged), "not connect onTransactionStatusChanged");
    CHECK(connect(this, &TransactionsJavascript::transactionStatusChanged2Sig, this, &TransactionsJavascript::onTransactionStatusChanged2), "not connect onTransactionStatusChanged2");

    Q_REG(TransactionsJavascript::Callback, "TransactionsJavascript::Callback");

    Q_REG(BalanceInfo, "BalanceInfo");
    Q_REG(Transaction, "Transaction");

    signalFunc = std::bind(&TransactionsJavascript::callbackCall, this, _1);
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
    //LOG << "Javascript " << script;
    emit jsRunSig(script);
}

static QJsonObject balanceToJson1(const BalanceInfo &balance) {
    QJsonObject messagesBalanceJson;
    messagesBalanceJson.insert("received", QString(balance.received.getDecimal()));
    messagesBalanceJson.insert("spent", QString(balance.spent.getDecimal()));
    messagesBalanceJson.insert("countReceived", QString::fromStdString(std::to_string(balance.countReceived)));
    messagesBalanceJson.insert("countSpent", QString::fromStdString(std::to_string(balance.countSpent)));
    messagesBalanceJson.insert("currBlock", QString::fromStdString(std::to_string(balance.currBlockNum)));
    messagesBalanceJson.insert("countDelegated", QString::fromStdString(std::to_string(balance.countDelegated)));
    messagesBalanceJson.insert("delegate", QString(balance.delegate.getDecimal()));
    messagesBalanceJson.insert("undelegate", QString(balance.undelegate.getDecimal()));
    messagesBalanceJson.insert("delegated", QString(balance.delegated.getDecimal()));
    messagesBalanceJson.insert("undelegated", QString(balance.undelegated.getDecimal()));
    messagesBalanceJson.insert("reserved", QString(balance.reserved.getDecimal()));
    messagesBalanceJson.insert("forged", QString(balance.forged.getDecimal()));
    messagesBalanceJson.insert("balance", QString(balance.calcBalance().getDecimal()));
    return messagesBalanceJson;
}

static QJsonDocument balanceToJson(const BalanceInfo &balance) {
    return QJsonDocument(balanceToJson1(balance));
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
    txJson.insert("blockNumber", QString::fromStdString(std::to_string(tx.blockNumber)));
    txJson.insert("intStatus", tx.intStatus);
    if (tx.isSetDelegate) {
        txJson.insert("isDelegate", tx.isDelegate);
        txJson.insert("delegate_value", tx.delegateValue);
        if (!tx.delegateHash.isEmpty()) {
            txJson.insert("delegate_hash", tx.delegateHash);
        }
    }

    QString statusStr;
    if (tx.status == Transaction::OK) {
        statusStr = "ok";
    } else if (tx.status == Transaction::PENDING) {
        statusStr = "pending";
    } else if (tx.status == Transaction::ERROR) {
        statusStr = "error";
    } else if (tx.status == Transaction::MODULE_NOT_SET) {
        statusStr = "module_not_set";
    } else {
        throwErr("Incorrect transaction status " + std::to_string(tx.status));
    }
    txJson.insert("status", statusStr);

    QString typeStr;
    if (tx.type == Transaction::SIMPLE) {
        typeStr = "simple";
    } else if (tx.type == Transaction::DELEGATE) {
        typeStr = "delegate";
    } else if (tx.type == Transaction::FORGING) {
        typeStr = "forging";
    } else {
        throwErr("Incorrect transaction type " + std::to_string(tx.type));
    }
    txJson.insert("type", typeStr);
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

        txJson.insert("balance", balanceToJson1(info.balance));

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

    LOG << "New balance " << address << " " << currency << " " << balance.countReceived << " " << balance.countSpent << " " << QString(balance.calcBalance().getDecimal());

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, currency, balanceToJson(balance));
END_SLOT_WRAPPER
}

void TransactionsJavascript::registerAddress(QString address, QString currency, QString type, QString group, QString name) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsRegisterAddressJs";

    LOG << "Txs register address " << address << " " << currency << " " << type << " " << group;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency);
    };

    const auto errorFunc = [makeFunc, address, currency](const TypedException &e) {
        makeFunc(e, address, currency);
    };

    const TypedException exception = apiVrapper2([&, this](){
        AddressInfo info(currency, address, type, group, name);
        emit transactionsManager->registerAddresses({info}, Transactions::RegisterAddressCallback([address, currency, makeFunc]() {
            LOG << "Txs register address ok " << address << " " << currency;
            makeFunc(TypedException(), address, currency);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
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

    LOG << "Txs register addresses " << infos.size();

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, QString("Ok"));
    };

    const auto errorFunc = [JS_NAME_RESULT, this](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, QString("Not ok"));
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->registerAddresses(infos, Transactions::RegisterAddressCallback([makeFunc]() {
            LOG << "Txs register addresses ok";
            makeFunc(TypedException());
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getAddresses(QString group) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetAddressesResultJs";

    LOG << "Txs get addresses " << group;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const auto errorFunc = [makeFunc](const TypedException &e) {
        makeFunc(e, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getAddresses(group, Transactions::GetAddressesCallback([makeFunc](const std::vector<AddressInfo> &infos) {
            LOG << "Txs get addresses ok " << infos.size();
            const QJsonDocument &result = addressInfoToJson(infos);
            makeFunc(TypedException(), result);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::setCurrentGroup(QString group) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsSetCurrentGroupResultJs";

    LOG << "Txs Set group " << group;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, QString("Ok"));
    };

    const auto errorFunc = [JS_NAME_RESULT, this](const TypedException &e) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, e, QString("Not ok"));
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->setCurrentGroup(group, Transactions::SetCurrentGroupCallback([makeFunc, group]() {
            LOG << "Txs Set group ok " << group;
            makeFunc(TypedException());
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxs(QString address, QString currency, QString fromTx, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsJs";

    LOG << "get txs address " << address << " " << currency << " " << fromTx << " " << count << " " << asc;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const auto errorFunc = [makeFunc, address, currency](const TypedException &e) {
        makeFunc(e, address, currency, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxs(address, currency, fromTx, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs address ok " << address << " " << currency << " " << txs.size();
            makeFunc(TypedException(), address, currency, txsToJson(txs));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxsAll(QString currency, QString fromTx, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsAllJs";

    LOG << "get txs address " << currency << " " << fromTx << " " << count << " " << asc;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, currency, result);
    };

    const auto errorFunc = [makeFunc, currency](const TypedException &e) {
        makeFunc(e, currency, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxsAll(currency, fromTx, count, asc, Transactions::GetTxsCallback([currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs address ok " << currency << " " << txs.size();
            makeFunc(TypedException(), currency, txsToJson(txs));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxs2(QString address, QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxs2Js";

    LOG << "get txs2 address " << address << " " << currency << " " << from << " " << count << " " << asc;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const auto errorFunc = [makeFunc, address, currency](const TypedException &e) {
        makeFunc(e, address, currency, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxs2(address, currency, from, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs2 address ok " << address << " " << currency << " " << txs.size();
            makeFunc(TypedException(), address, currency, txsToJson(txs));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxsAll2(QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsAll2Js";

    LOG << "get txs2 address " << currency << " " << from << " " << count << " " << asc;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, currency, result);
    };

    const auto errorFunc = [makeFunc, currency](const TypedException &e) {
        makeFunc(e, currency, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxsAll2(currency, from, count, asc, Transactions::GetTxsCallback([currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs2 address ok " << currency << " " << txs.size();
            makeFunc(TypedException(), currency, txsToJson(txs));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
    END_SLOT_WRAPPER
}

void TransactionsJavascript::getForgingTxsAll(QString address, QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetForgingTxsJs";

    LOG << "get forging txs address " << address << " " << currency << " " << from << " " << count << " " << asc;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const auto errorFunc = [makeFunc, address, currency](const TypedException &e) {
        makeFunc(e, address, currency, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this]() {
        emit transactionsManager->getForgingTxs(address, currency, from, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get forging txs address ok " << address << " " << currency << " " << txs.size();
            makeFunc(TypedException(), address, currency, txsToJson(txs));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getLastForgingTx(QString address, QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetLastForgingTxJs";

    LOG << "get forging tx address " << address << " " << currency;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const auto errorFunc = [makeFunc, address, currency](const TypedException &e) {
        makeFunc(e, address, currency, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this]() {
        emit transactionsManager->getLastForgingTx(address, currency, Transactions::GetTxCallback([address, currency, makeFunc](const Transaction &txs) {
            LOG << "get forging tx address ok " << address << " " << currency << " " << txs.tx;
            makeFunc(TypedException(), address, currency, txInfoToJson(txs));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::calcBalance(QString address, QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsCalcBalanceResultJs";

    LOG << "calc balance address " << currency << " " << address;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &currency, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, currency, result);
    };

    const auto errorFunc = [makeFunc, address, currency](const TypedException &e) {
        makeFunc(e, address, currency, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->calcBalance(address, currency, Transactions::CalcBalanceCallback([currency, address, makeFunc](const BalanceInfo &balance) {
            LOG << "calc balance ok " << currency << " " << address << " " << QString(balance.calcBalance().getDecimal()) << " " << balance.countReceived << " " << balance.countSpent;
            makeFunc(TypedException(), address, currency, balanceToJson(balance));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxFromServer(QString txHash, QString type) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxFromServerResultJs";

    LOG << "getTxFromServer address " << txHash << " " << type;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &txHash, const QString &type, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, txHash, type, result);
    };

    const auto errorFunc = [makeFunc, txHash, type](const TypedException &e) {
        makeFunc(e, txHash, type, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getTxFromServer(txHash, type, Transactions::GetTxCallback([txHash, type, makeFunc](const Transaction &tx) {
            LOG << "getTxFromServer address ok " << txHash << " " << type;
            makeFunc(TypedException(), txHash, type, txInfoToJson(tx));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::getLastUpdatedBalance(QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetLastUpdatedBalanceResultJs";

    LOG << "getLastUpdatedBalance " << currency;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &currency, const QString &timestamp, const QString &now) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, currency, timestamp, now);
    };

    const auto errorFunc = [makeFunc, currency](const TypedException &e) {
        makeFunc(e, currency, "", "");
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->getLastUpdateBalance(currency, Transactions::GetLastUpdateCallback([currency, makeFunc](const system_time_point &result, const system_time_point &now) {
            const QString r = QString::fromStdString(std::to_string(systemTimePointToMilliseconds(result)));
            const QString n = QString::fromStdString(std::to_string(systemTimePointToMilliseconds(now)));
            LOG << "Get getLastUpdateBalance ok " << r << " " << n;
            makeFunc(TypedException(), currency, r, n);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::clearDb(QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsClearDbResultJs";

    LOG << "clear db " << currency;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &currency, const std::string &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, currency, result);
    };

    const auto errorFunc = [makeFunc, currency](const TypedException &e) {
        makeFunc(e, currency, "Not ok");
    };

    const TypedException exception = apiVrapper2([&, this](){
        emit transactionsManager->clearDb(currency, Transactions::ClearDbCallback([currency, makeFunc]() {
            makeFunc(TypedException(), currency, "Ok");
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void TransactionsJavascript::onSendedTransactionsResponse(const QString &requestId, const QString &server, const QString &response, const TypedException &error) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "txsSendedTxJs";
    LOG << "Transaction sended " << server << " " << response << " " << error.description;
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, requestId, server, response);
END_SLOT_WRAPPER
}

void TransactionsJavascript::onTransactionInTorrent(const QString &requestId, const QString &server, const QString &txHash, const Transaction &tx, const TypedException &error) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "txOnTorrentJs";
    LOG << "Transaction on torrent " << server << " " << txHash << " " << error.description;
    makeAndRunJsFuncParams(JS_NAME_RESULT, error, server, txHash, txInfoToJson(tx));
END_SLOT_WRAPPER
}

void TransactionsJavascript::onTransactionStatusChanged(const QString &address, const QString &currency, const QString &txHash, const Transaction &tx) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "txStatusChangedJs";
    LOG << "Transaction status changed " << txHash;
    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, currency, txHash, txInfoToJson(tx));
END_SLOT_WRAPPER
}

void TransactionsJavascript::onTransactionStatusChanged2(const QString &txHash, const Transaction &tx) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "txStatusChanged2Js";
    LOG << "Transaction status changed2 " << txHash;
    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), txHash, txInfoToJson(tx));
END_SLOT_WRAPPER
}

}
