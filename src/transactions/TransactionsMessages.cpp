#include "TransactionsMessages.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"
#include "Log.h"

#include "Transaction.h"

namespace transactions {

QString makeGetBalanceRequest(const QString &address) {
    return "{\"id\":1,\"params\":{\"address\": \"" + address + "\"},\"method\":\"fetch-balance\", \"pretty\": false}";
}

static QString getIntOrString(const QJsonObject &json, const QString &key) {
    CHECK(json.contains(key), "Incorrect json: " + key.toStdString() + " field not found");
    if (json.value(key).isDouble()) {
        return QString::fromStdString(std::to_string(uint64_t(json.value(key).toDouble())));
    } else if (json.value(key).isString()) {
        return json.value(key).toString();
    } else {
        throwErr("Incorrect json: " + key.toStdString() + " field not found");
    }
}

BalanceInfo parseBalanceResponse(const QString &response) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK(json1.contains("result") && json1.value("result").isObject(), "Incorrect json: result field not found");
    const QJsonObject &json = json1.value("result").toObject();

    BalanceInfo result;

    CHECK(json.contains("address") && json.value("address").isString(), "Incorrect json: address field not found");
    result.address = json.value("address").toString();
    result.received = getIntOrString(json, "received");
    result.spent = getIntOrString(json, "spent");
    result.countReceived = getIntOrString(json, "count_received").toULong();
    result.countSpent = getIntOrString(json, "count_spent").toULong();
    result.currBlockNum = getIntOrString(json, "currentBlock").toULong();

    if (json.contains("countDelegatedOps")) {
        result.countDelegated = getIntOrString(json, "countDelegatedOps").toULong();
        result.delegate = getIntOrString(json, "delegate");
        result.undelegate = getIntOrString(json, "undelegate");
        result.delegated = getIntOrString(json, "delegated");
        result.undelegated = getIntOrString(json, "undelegated");
        if (json.contains("reserved")) {
            result.reserved = getIntOrString(json, "reserved");
        } else {
            result.reserved = QString("0");
        }
    }

    if (json.contains("forged")) {
        result.forged = getIntOrString(json, "forged");
    }

    return result;
}

QString makeGetHistoryRequest(const QString &address, bool isCnt, uint64_t cnt) {
    if (isCnt) {
        return "{\"id\":1,\"params\":{\"address\": \"" + address + "\"},\"method\":\"fetch-history\", \"pretty\": false}";
    } else {
        return "{\"id\":1,\"params\":{\"address\": \"" + address + "\", \"countTxs\": " + QString::fromStdString(std::to_string(cnt)) + "},\"method\":\"fetch-history\", \"pretty\": false}";
    }
}

QString makeGetTxRequest(const QString &hash) {
    return "{\"id\":1,\"params\":{\"hash\": \"" + hash + "\"},\"method\":\"get-tx\", \"pretty\": false}";
}

static Transaction parseTransaction(const QJsonObject &txJson, const QString &address, const QString &currency) {
    Transaction res;

    CHECK(txJson.contains("from") && txJson.value("from").isString(), "Incorrect json: from field not found");
    res.from = txJson.value("from").toString();
    CHECK(txJson.contains("to") && txJson.value("to").isString(), "Incorrect json: to field not found");
    res.to = txJson.value("to").toString();
    res.value = getIntOrString(txJson, "value");
    CHECK(txJson.contains("transaction") && txJson.value("transaction").isString(), "Incorrect json: transaction field not found");
    res.tx = txJson.value("transaction").toString();
    if (txJson.contains("data") && txJson.value("data").isString()) {
        res.data = txJson.value("data").toString();
    }
    res.timestamp = getIntOrString(txJson, "timestamp").toULongLong();
    if (txJson.contains("realFee")) {
        res.fee = getIntOrString(txJson, "realFee");
    } else if (txJson.contains("fee")) {
        res.fee = getIntOrString(txJson, "fee");
    }
    if (res.fee.isEmpty() || res.fee.isNull()) {
        res.fee = "0";
    }
    if (txJson.contains("nonce")) {
        res.nonce = getIntOrString(txJson, "nonce").toLong();
    }
    if (txJson.contains("isDelegate") && txJson.value("isDelegate").isBool()) {
        res.isSetDelegate = true;
        res.isDelegate = txJson.value("isDelegate").toBool();
        res.delegateValue = getIntOrString(txJson, "delegate");
        if (txJson.contains("delegateHash") && txJson.value("delegateHash").isString()) {
            res.delegateHash = txJson.value("delegateHash").toString();
        }
        res.type = Transaction::DELEGATE;
    }
    if (txJson.contains("status") && txJson.value("status").isString()) {
        const QString status = txJson.value("status").toString();
        if (status == "ok") {
            res.status = Transaction::OK;
        } else if (status == "error") {
            res.status = Transaction::ERROR;
        } else if (status == "pending") {
            res.status = Transaction::PENDING;
        } else if (status == "module_not_set") {
            res.status = Transaction::MODULE_NOT_SET;
        }
    }

    if (txJson.contains("blockNumber")) {
        res.blockNumber = getIntOrString(txJson, "blockNumber").toLong();
    }

    if (txJson.contains("type") && txJson.value("type").isString() && txJson.value("type").toString() == "forging") {
        res.type = Transaction::FORGING;
    }

    res.address = address;
    res.currency = currency;
    res.isInput = res.address == res.from;
    return res;
}

std::vector<Transaction> parseHistoryResponse(const QString &address, const QString &currency, const QString &response) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK(json1.contains("result") && json1.value("result").isArray(), "Incorrect json: result field not found");
    const QJsonArray &json = json1.value("result").toArray();

    std::vector<Transaction> result;

    for (const QJsonValue &elementJson: json) {
        CHECK(elementJson.isObject(), "Incorrect json");
        const QJsonObject txJson = elementJson.toObject();

        const Transaction res = parseTransaction(txJson, address, currency);

        result.emplace_back(res);
        if (res.from == address && res.to == address) {
            Transaction res2 = res;
            res2.isInput = false;
            result.emplace_back(res2);
        }
    }

    return result;
}

QString makeSendTransactionRequest(const QString &to, const QString &value, size_t nonce, const QString &data, const QString &fee, const QString &pubkey, const QString &sign) {
    QJsonObject request;
    request.insert("jsonrpc", "2.0");
    request.insert("method", "mhc_send");
    QJsonObject params;
    params.insert("to", to);
    params.insert("value", value);
    params.insert("nonce", QString::fromStdString(std::to_string(nonce)));
    params.insert("data", data);
    params.insert("fee", fee);
    params.insert("pubkey", pubkey);
    params.insert("sign", sign);
    request.insert("params", params);
    return QString(QJsonDocument(request).toJson(QJsonDocument::Compact));
}

QString parseSendTransactionResponse(const QString &response) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK_TYPED(!json1.contains("error") || !json1.value("error").isString(), TypeErrors::TRANSACTIONS_SERVER_SEND_ERROR, json1.value("error").toString().toStdString());

    CHECK(json1.contains("params") && json1.value("params").isString(), "Incorrect json: params field not found");
    return json1.value("params").toString();
}

Transaction parseGetTxResponse(const QString &response, const QString &address, const QString &currency) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK(!json1.contains("error") || !json1.value("error").isObject(), json1.value("error").toObject().value("message").toString().toStdString());

    CHECK(json1.contains("result") && json1.value("result").isObject(), "Incorrect json: result field not found");
    const QJsonObject &obj = json1.value("result").toObject();
    CHECK(obj.contains("transaction") && obj.value("transaction").isObject(), "Incorrect json: transaction field not found");
    const QJsonObject &transaction = obj.value("transaction").toObject();
    return parseTransaction(transaction, address, currency);
}

QString makeGetBlockInfoRequest(int64_t blockNumber) {
    QJsonObject request;
    request.insert("jsonrpc", "2.0");
    request.insert("method", "get-block-by-number");
    QJsonObject params;
    params.insert("number", (int)blockNumber);
    params.insert("type", 0); // TODO заменить на small
    request.insert("params", params);
    return QString(QJsonDocument(request).toJson(QJsonDocument::Compact));
}

BlockInfo parseGetBlockInfoResponse(const QString &response) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK(!json1.contains("error") || !json1.value("error").isObject(), json1.value("error").toObject().value("message").toString().toStdString());

    CHECK(json1.contains("result") && json1.value("result").isObject(), "Incorrect json: result field not found");
    const QJsonObject &obj = json1.value("result").toObject();

    BlockInfo result;
    CHECK(obj.contains("hash") && obj.value("hash").isString(), "Incorrect json: hash field not found");
    result.hash = obj.value("hash").toString();
    CHECK(obj.contains("number") && obj.value("number").isDouble(), "Incorrect json: number field not found");
    result.number = obj.value("number").toInt();

    return result;
}

QString makeGetCountBlocksRequest() {
    QJsonObject request;
    request.insert("jsonrpc", "2.0");
    request.insert("method", "get-count-blocks");
    QJsonObject params;
    request.insert("params", params);
    return QString(QJsonDocument(request).toJson(QJsonDocument::Compact));
}

int64_t parseGetCountBlocksResponse(const QString &response) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK(!json1.contains("error") || !json1.value("error").isObject(), json1.value("error").toObject().value("message").toString().toStdString());

    CHECK(json1.contains("result") && json1.value("result").isObject(), "Incorrect json: result field not found");
    const QJsonObject &obj = json1.value("result").toObject();

    CHECK(obj.contains("count_blocks") && obj.value("count_blocks").isDouble(), "Incorrect json: count_blocks field not found");
    return obj.value("count_blocks").toInt();
}

}
