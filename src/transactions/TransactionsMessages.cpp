#include "TransactionsMessages.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"

namespace transactions {

QString makeGetBalanceRequest(const QString &address) {
    return "{\"id\":1,\"params\":{\"address\": \"" + address + "\"},\"method\":\"fetch-balance\", \"pretty\": false}";
}

BalanceResponse parseBalanceResponse(const QString &response) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK(json1.contains("result") && json1.value("result").isObject(), "Incorrect json: result field not found");
    const QJsonObject &json = json1.value("result").toObject();

    BalanceResponse result;

    CHECK(json.contains("address") && json.value("address").isString(), "Incorrect json: address field not found");
    result.address = json.value("address").toString();
    CHECK(json.contains("received") && json.value("received").isDouble(), "Incorrect json: received field not found");
    result.received = QString::fromStdString(std::to_string(uint64_t(json.value("received").toDouble())));
    CHECK(json.contains("spent") && json.value("spent").isDouble(), "Incorrect json: spent field not found");
    result.spent = QString::fromStdString(std::to_string(uint64_t(json.value("spent").toDouble())));
    CHECK(json.contains("count_received") && json.value("count_received").isDouble(), "Incorrect json: count_received field not found");
    result.countReceived = json.value("count_received").toDouble();
    CHECK(json.contains("count_spent") && json.value("count_spent").isDouble(), "Incorrect json: count_spent field not found");
    result.countSpent = json.value("count_spent").toDouble();
    CHECK(json.contains("currentBlock") && json.value("currentBlock").isDouble(), "Incorrect json: currentBlock field not found");
    result.currBlockNum = json.value("currentBlock").toDouble();

    return result;
}

QString makeGetHistoryRequest(const QString &address, bool isCnt, uint64_t cnt) {
    if (isCnt) {
        return "{\"id\":1,\"params\":{\"address\": \"" + address + "\"},\"method\":\"fetch-history\", \"pretty\": false}";
    } else {
        return "{\"id\":1,\"params\":{\"address\": \"" + address + "\", \"countTxs\": " + QString::fromStdString(std::to_string(cnt)) + "},\"method\":\"fetch-history\", \"pretty\": false}";
    }
}

std::vector<Transaction> parseHistoryResponse(const QString &address, const QString &response) {
    const QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    CHECK(jsonResponse.isObject(), "Incorrect json ");
    const QJsonObject &json1 = jsonResponse.object();
    CHECK(json1.contains("result") && json1.value("result").isArray(), "Incorrect json: result field not found");
    const QJsonArray &json = json1.value("result").toArray();

    std::vector<Transaction> result;

    for (const QJsonValue &elementJson: json) {
        CHECK(elementJson.isObject(), "Incorrect json");
        const QJsonObject txJson = elementJson.toObject();

        Transaction res;

        CHECK(txJson.contains("from") && txJson.value("from").isString(), "Incorrect json: from field not found");
        res.from = txJson.value("from").toString();
        CHECK(txJson.contains("to") && txJson.value("to").isString(), "Incorrect json: to field not found");
        res.to = txJson.value("to").toString();
        CHECK(txJson.contains("value") && txJson.value("value").isDouble(), "Incorrect json: value field not found");
        res.value = QString::fromStdString(std::to_string(uint64_t(txJson.value("value").toDouble())));
        CHECK(txJson.contains("transaction") && txJson.value("transaction").isString(), "Incorrect json: transaction field not found");
        res.tx = txJson.value("transaction").toString();
        CHECK(txJson.contains("data") && txJson.value("data").isString(), "Incorrect json: data field not found");
        res.data = txJson.value("data").toString();
        CHECK(txJson.contains("timestamp") && txJson.value("timestamp").isDouble(), "Incorrect json: timestamp field not found");
        res.timestamp = txJson.value("timestamp").toDouble();
        CHECK(txJson.contains("fee") && txJson.value("fee").isDouble(), "Incorrect json: fee field not found");
        res.fee = txJson.value("fee").toDouble();
        CHECK(txJson.contains("nonce") && txJson.value("nonce").isDouble(), "Incorrect json: nonce field not found");
        res.nonce = txJson.value("nonce").toInt();

        res.isInput = res.from == address;

        result.emplace_back(res);
        if (res.from == address && res.to == address) {
            Transaction res2 = res;
            res2.isInput = false;
            result.emplace_back(res2);
        }
    }

    return result;
}

}
