#include "TransactionsJavascript.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "Transaction.h"

#include "qt_utilites/makeJsFunc.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

#include "Transactions.h"
#include "TransactionsFilter.h"

#include "qt_utilites/WrapperJavascriptImpl.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("TXS");

namespace transactions {

TransactionsJavascript::TransactionsJavascript(QObject *parent)
    : WrapperJavascript(false, LOG_FILE)
{
    Q_CONNECT(this, &TransactionsJavascript::newBalanceSig, this, &TransactionsJavascript::onNewBalance);
    Q_CONNECT(this, &TransactionsJavascript::sendedTransactionsResponseSig, this, &TransactionsJavascript::onSendedTransactionsResponse);
    Q_CONNECT(this, &TransactionsJavascript::transactionInTorrentSig, this, &TransactionsJavascript::onTransactionInTorrent);
    Q_CONNECT(this, &TransactionsJavascript::transactionStatusChangedSig, this, &TransactionsJavascript::onTransactionStatusChanged);
    Q_CONNECT(this, &TransactionsJavascript::transactionStatusChanged2Sig, this, &TransactionsJavascript::onTransactionStatusChanged2);

    Q_REG(BalanceInfo, "BalanceInfo");
    Q_REG(Transaction, "Transaction");
}

static QJsonObject balanceToJson1(const BalanceInfo &balance) {
    QJsonObject messagesBalanceJson;
    messagesBalanceJson.insert("received", QString(balance.received.getDecimal()));
    messagesBalanceJson.insert("spent", QString(balance.spent.getDecimal()));
    messagesBalanceJson.insert("countReceived", QString::fromStdString(std::to_string(balance.countReceived)));
    messagesBalanceJson.insert("countSpent", QString::fromStdString(std::to_string(balance.countSpent)));
    messagesBalanceJson.insert("countTxs", QString::fromStdString(std::to_string(balance.countTxs)));
    messagesBalanceJson.insert("savedTxs", QString::fromStdString(std::to_string(balance.savedTxs)));
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
    txJson.insert("blockNumber", QString::fromStdString(std::to_string(tx.blockNumber)));
    txJson.insert("blockIndex", QString::fromStdString(std::to_string(tx.blockIndex)));
    txJson.insert("intStatus", tx.intStatus);
    if (tx.type == Transaction::Type::DELEGATE) {
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
    } else if (tx.type == Transaction::CONTRACT) {
        typeStr = "contract";
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

    LOG << "New balance " << address << " " << currency << " " << balance.countTxs << " " << QString(balance.calcBalance().getDecimal());

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, currency, balanceToJson(balance));
END_SLOT_WRAPPER
}

void TransactionsJavascript::registerAddress(QString address, QString currency, QString type, QString group, QString name) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsRegisterAddressJs";

    LOG << "Register address " << address << " " << currency << " " << type << " " << group;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency));

    wrapOperation([&, this](){
        AddressInfo info(currency, address, group);
        emit transactionsManager->registerAddresses({info}, Transactions::RegisterAddressCallback([address, currency, makeFunc]() {
            LOG << "Register address ok " << address << " " << currency;
            makeFunc.func(TypedException(), address, currency);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
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
        CHECK(addressJson.contains("group") && addressJson.value("group").isString(), "Incorrect json: group field not found");
        addressInfo.group = addressJson.value("group").toString();

        infos.emplace_back(addressInfo);
    }

    LOG << "Register addresses " << infos.size();

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit transactionsManager->registerAddresses(infos, Transactions::RegisterAddressCallback([makeFunc]() {
            LOG << "Register addresses ok";
            makeFunc.func(TypedException(), "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getAddresses(QString group) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetAddressesResultJs";

    LOG << "Get addresses " << group;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit transactionsManager->getAddresses(group, Transactions::GetAddressesCallback([makeFunc](const std::vector<AddressInfo> &infos) {
            LOG << "Get addresses ok " << infos.size();
            const QJsonDocument &result = addressInfoToJson(infos);
            makeFunc.func(TypedException(), result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::setCurrentGroup(QString group) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsSetCurrentGroupResultJs";

    LOG << "Set group " << group;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit transactionsManager->setCurrentGroup(group, Transactions::SetCurrentGroupCallback([makeFunc, group]() {
            LOG << "Set group ok " << group;
            makeFunc.func(TypedException(), "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxs(QString address, QString currency, QString fromTx, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsJs";

    LOG << "get txs address " << address << " " << currency << " " << fromTx << " " << count << " " << asc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit transactionsManager->getTxs(address, currency, fromTx, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs address ok " << address << " " << currency << " " << txs.size();
            makeFunc.func(TypedException(), address, currency, txsToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxsAll(QString currency, QString fromTx, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsAllJs";

    LOG << "get txs address " << currency << " " << fromTx << " " << count << " " << asc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit transactionsManager->getTxsAll(currency, fromTx, count, asc, Transactions::GetTxsCallback([currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs address ok " << currency << " " << txs.size();
            makeFunc.func(TypedException(), currency, txsToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxs2(QString address, QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxs2Js";

    LOG << "get txs2 address " << address << " " << currency << " " << from << " " << count << " " << asc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit transactionsManager->getTxs2(address, currency, from, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs2 address ok " << address << " " << currency << " " << txs.size();
            makeFunc.func(TypedException(), address, currency, txsToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxsAll2(QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsAll2Js";

    LOG << "get txs2 address " << currency << " " << from << " " << count << " " << asc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit transactionsManager->getTxsAll2(currency, from, count, asc, Transactions::GetTxsCallback([currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs2 address ok " << currency << " " << txs.size();
            makeFunc.func(TypedException(), currency, txsToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

static Filters jsonToFilters(const QString &filtersJson) {
    Filters filters;

    const auto boolToType = [](bool b) {
        if (b) {
            return FilterType::True;
        } else {
            return FilterType::False;
        }
    };

    const QJsonDocument jsonResponse = QJsonDocument::fromJson(filtersJson.toUtf8());
    CHECK(jsonResponse.isArray(), "Incorrect json ");
    const QJsonArray &json1 = jsonResponse.array();
    for (const QJsonValue &val: json1) {
        CHECK(val.isObject(), "Incorrect json");
        const QJsonObject filterJson = val.toObject();
        CHECK(filterJson.contains("name") && filterJson.value("name").isString(), "name field not found");
        const QString name = filterJson.value("name").toString();
        CHECK(filterJson.contains("value") && filterJson.value("value").isBool(), "value field not found");
        const bool value = filterJson.value("value").toBool();

        if (name == "isInput") {
            filters.isInput = boolToType(value);
        } else if (name == "isOutput") {
            filters.isOutput = boolToType(value);
        } else if (name == "isDelegate") {
            filters.isDelegate = boolToType(value);
        } else if (name == "isForging") {
            filters.isForging = boolToType(value);
        } else if (name == "isSuccess") {
            filters.isSuccess = boolToType(value);
        } else if (name == "isTesting") {
            filters.isTesting = boolToType(value);
        }
    }

    return filters;
};

void TransactionsJavascript::getTxsFilters(QString address, QString currency, QString filtersJson, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxsFiltersJs";

    LOG << "get txs filters address " << address << " " << currency << " " << from << " " << count << " " << asc << " " << filtersJson;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit transactionsManager->getTxsFilters(address, currency, jsonToFilters(filtersJson), from, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get txs filters address ok " << address << " " << currency << " " << txs.size();
            makeFunc.func(TypedException(), address, currency, txsToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getForgingTxsAll(QString address, QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetForgingTxsJs";

    LOG << "get forging txs address " << address << " " << currency << " " << from << " " << count << " " << asc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this]() {
        emit transactionsManager->getForgingTxs(address, currency, from, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get forging txs address ok " << address << " " << currency << " " << txs.size();
            makeFunc.func(TypedException(), address, currency, txsToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getDelegateTxsAll(QString address, QString currency, QString to, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetDelegateTxsJs";

    LOG << "get delegate txs address " << address << " " << currency << " " << to << " " << from << " " << count << " " << asc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this]() {
        emit transactionsManager->getDelegateTxs(address, currency, to, from, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get delegate txs address ok " << address << " " << currency << " " << txs.size();
            makeFunc.func(TypedException(), address, currency, txsToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getDelegateTxsAll2(QString address, QString currency, int from, int count, bool asc) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetDelegateTxs2Js";

    LOG << "get delegate txs address " << address << " " << currency << " " << from << " " << count << " " << asc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this]() {
        emit transactionsManager->getDelegateTxs2(address, currency, from, count, asc, Transactions::GetTxsCallback([address, currency, makeFunc](const std::vector<Transaction> &txs) {
            LOG << "get delegate txs address ok " << address << " " << currency << " " << txs.size();
            makeFunc.func(TypedException(), address, currency, txsToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getLastForgingTx(QString address, QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetLastForgingTxJs";

    LOG << "get forging tx address " << address << " " << currency;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this]() {
        emit transactionsManager->getLastForgingTx(address, currency, Transactions::GetTxCallback([address, currency, makeFunc](const Transaction &txs) {
            LOG << "get forging tx address ok " << address << " " << currency << " " << txs.tx;
            makeFunc.func(TypedException(), address, currency, txInfoToJson(txs));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::calcBalance(const QString &address, const QString &currency, const QString &callback) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsCalcBalanceResultJs"; // TODO deprecated

    LOG << "calc balance address " << currency << " " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(chooseCallback(callback, JS_NAME_RESULT), JsTypeReturn<QString>(address), JsTypeReturn<QString>(currency), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit transactionsManager->calcBalance(address, currency, Transactions::CalcBalanceCallback([currency, address, makeFunc](const BalanceInfo &balance) {
            LOG << "calc balance ok " << currency << " " << address << " " << QString(balance.calcBalance().getDecimal()) << " " << balance.countTxs << " " << balance.savedTxs;
            makeFunc.func(TypedException(), address, currency, balanceToJson(balance));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getTxFromServer(QString txHash, QString type) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetTxFromServerResultJs";

    LOG << "getTxFromServer address " << txHash << " " << type;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(txHash), JsTypeReturn<QString>(type), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit transactionsManager->getTxFromServer(txHash, type, Transactions::GetTxCallback([txHash, type, makeFunc](const Transaction &tx) {
            LOG << "getTxFromServer address ok " << txHash << " " << type;
            makeFunc.func(TypedException(), txHash, type, txInfoToJson(tx));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::getLastUpdatedBalance(QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsGetLastUpdatedBalanceResultJs";

    LOG << "getLastUpdatedBalance " << currency;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(currency), JsTypeReturn<QString>(""), JsTypeReturn<QString>(""));

    wrapOperation([&, this](){
        emit transactionsManager->getLastUpdateBalance(currency, Transactions::GetLastUpdateCallback([currency, makeFunc](const system_time_point &result, const system_time_point &now) {
            const QString r = QString::fromStdString(std::to_string(systemTimePointToMilliseconds(result)));
            const QString n = QString::fromStdString(std::to_string(systemTimePointToMilliseconds(now)));
            LOG << "Get getLastUpdateBalance ok " << r << " " << n;
            makeFunc.func(TypedException(), currency, r, n);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::addCurrencyConformity(bool isMhc, QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsAddCurrencyConformityResultJs";

    LOG << "Add currency conformity " << currency << " " << isMhc;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this](){
        emit transactionsManager->addCurrencyConformity(isMhc, currency, Transactions::AddCurrencyConformity([currency, makeFunc]() {
            LOG << "Add currency conformity ok";
            makeFunc.func(TypedException(), "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void TransactionsJavascript::clearDb(QString currency) {
BEGIN_SLOT_WRAPPER
    CHECK(transactionsManager != nullptr, "transactions not set");

    const QString JS_NAME_RESULT = "txsClearDbResultJs";

    LOG << "clear db " << currency;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(currency), JsTypeReturn<std::string>("Not ok"));

    wrapOperation([&, this](){
        emit transactionsManager->clearDb(currency, Transactions::ClearDbCallback([currency, makeFunc]() {
            makeFunc.func(TypedException(), currency, "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
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
