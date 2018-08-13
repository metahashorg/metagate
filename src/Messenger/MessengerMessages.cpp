#include "MessengerMessages.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"

const static QString MSG_GET_MY_REQUEST = "msg_get_my";
const static QString MSG_GET_CHANNEL_REQUEST = "msg_get_channel";
const static QString MSG_GET_CHANNELS_REQUEST = "msg_get_channels";
const static QString MSG_APPEND_KEY_ONLINE_REQUEST = "msg_append_key_online";

const static QString APPEND_KEY_TO_ADDR_RESPONSE = "msg_append_key_to_addr";
const static QString GET_KEY_BY_ADDR_RESPONSE = "msg_get_key_by_addr";
const static QString SEND_TO_ADDR_RESPONSE = "msg_send_to_addr";
const static QString NEW_MSGS_RESPONSE = "msg_get_my";
const static QString NEW_MSG_RESPONSE = "new_msg";
const static QString COUNT_MESSAGES_RESPONSE = "msg_append_key_online";

QString makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee) {
    return address + QString::fromStdString(std::to_string(fee)) + rsaPubkeyHex;
}

QString makeTextForGetPubkeyRequest(const QString &address) {
    return address;
}

QString makeTextForSendMessageRequest(const QString &address, const QString &dataHex, uint64_t fee) {
    return address + QString::fromStdString(std::to_string(fee)) + dataHex;
}

QString makeTextForGetMyMessagesRequest() {
    return MSG_GET_MY_REQUEST;
}

QString makeTextForGetChannelRequest() {
    return MSG_GET_CHANNEL_REQUEST;
}

QString makeTextForGetChannelsRequest() {
    return MSG_GET_CHANNELS_REQUEST;
}

QString makeTextForMsgAppendKeyOnlineRequest() {
    return MSG_APPEND_KEY_ONLINE_REQUEST;
}

QString makeRegisterRequest(const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, size_t id) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", "msg_append_key_to_addr");
    json.insert("request_id", QString::fromStdString(std::to_string(id)));
    QJsonObject params;
    params.insert("fee", QString::fromStdString(std::to_string(fee)));
    params.insert("rsa_pub_key", rsaPubkeyHex);
    params.insert("pubkey", pubkeyAddressHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString makeGetPubkeyRequest(const QString &address, const QString &pubkeyHex, const QString &signHex, size_t id) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", "msg_get_key_by_addr");
    json.insert("request_id", QString::fromStdString(std::to_string(id)));
    QJsonObject params;
    params.insert("addr", address);
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString makeSendMessageRequest(const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, size_t id) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", "msg_send_to_addr");
    json.insert("request_id", QString::fromStdString(std::to_string(id)));
    QJsonObject params;
    params.insert("to", toAddress);
    params.insert("data", dataHex);
    params.insert("fee", QString::fromStdString(std::to_string(fee)));
    params.insert("timestamp", QString::fromStdString(std::to_string(timestamp)));
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString makeGetMyMessagesRequest(const QString &pubkeyHex, const QString &signHex, Message::Counter from, Message::Counter to, size_t id) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", MSG_GET_MY_REQUEST);
    json.insert("request_id", QString::fromStdString(std::to_string(id)));
    QJsonObject params;
    params.insert("cnt_from", QString::fromStdString(std::to_string(from)));
    params.insert("cnt_to", QString::fromStdString(std::to_string(to)));
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString makeAppendKeyOnlineRequest(const QString &pubkeyHex, const QString &signHex, size_t id) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", MSG_APPEND_KEY_ONLINE_REQUEST);
    json.insert("request_id", QString::fromStdString(std::to_string(id)));
    QJsonObject params;
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

ResponseType getMethodAndAddressResponse(const QJsonDocument &response) {
    ResponseType result;
    QString type;

    CHECK(response.isObject(), "Response field not found");
    const QJsonObject root = response.object();
    CHECK(root.contains("result") && root.value("result").isString(), "data field not found");
    const QString res = root.value("result").toString();
    CHECK(res == "ok", "result != ok"); // ok даже в случае ошибки
    if (root.contains("method") && root.value("method").isString()) {
        type = root.value("method").toString();
    }
    if (root.contains("addr") && root.value("addr").isString()) {
        result.address = root.value("addr").toString();
    }
    if (root.contains("error") && root.value("error").isString()) {
        result.error = root.value("error").toString();
        result.isError = true;
    }
    if (root.contains("request_id") && root.value("request_id").isString()) {
        result.id = std::stoull(root.value("request_id").toString().toStdString());
    }

    if (type == APPEND_KEY_TO_ADDR_RESPONSE) {
        result.method = METHOD::APPEND_KEY_TO_ADDR;
    } else if (type == GET_KEY_BY_ADDR_RESPONSE) {
        result.method = METHOD::GET_KEY_BY_ADDR;
    } else if (type == SEND_TO_ADDR_RESPONSE) {
        result.method = METHOD::SEND_TO_ADDR;
    } else if (type == NEW_MSGS_RESPONSE) {
        result.method = METHOD::NEW_MSGS;
    } else if (type == NEW_MSG_RESPONSE) {
        result.method = METHOD::NEW_MSG;
    } else if (type == COUNT_MESSAGES_RESPONSE) {
        result.method = METHOD::COUNT_MESSAGES;
    } else {
        throwErr(("Incorrect response: " + type).toStdString());
    }
    return result;
}

static NewMessageResponse parseOneMessage(const QJsonObject &messageJson) {
    NewMessageResponse result;
    CHECK(messageJson.contains("from") && messageJson.value("from").isString(), "from field not found");
    result.collocutor = messageJson.value("from").toString();
    CHECK(messageJson.contains("data") && messageJson.value("data").isString(), "data field not found");
    result.data = messageJson.value("data").toString();
    CHECK(messageJson.contains("fee") && messageJson.value("fee").isString(), "fee field not found");
    result.fee = std::stoull(messageJson.value("fee").toString().toStdString());
    CHECK(messageJson.contains("cnt") && messageJson.value("cnt").isString(), "cnt field not found");
    result.counter = std::stoll(messageJson.value("cnt").toString().toStdString());
    CHECK(messageJson.contains("timestamp") && messageJson.value("timestamp").isString(), "timestamp field not found");
    result.timestamp = std::stoull(messageJson.value("timestamp").toString().toStdString());
    CHECK(messageJson.contains("type") && messageJson.value("type").isString(), "type field not found");
    const QString type = messageJson.value("type").toString();
    CHECK(type == "in" || type == "out", "type field incorrect");
    result.isInput = type == "in";

    return result;
}

NewMessageResponse parseNewMessageResponse(const QJsonDocument &response) {
    CHECK(response.isObject(), "Response field not found");
    const QJsonObject root = response.object();
    CHECK(root.contains("params") && root.value("params").isObject(), "params field not found");
    const QJsonObject data = root.value("params").toObject();

    return parseOneMessage(data);
}

std::vector<NewMessageResponse> parseNewMessagesResponse(const QJsonDocument &response) {
    CHECK(response.isObject(), "Response field not found");
    const QJsonObject root = response.object();
    CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
    const QJsonObject data = root.value("data").toObject();
    CHECK(data.contains("messages") && data.value("messages").isArray(), "messages field not found");
    const QJsonArray messages = data.value("messages").toArray();

    std::vector<NewMessageResponse> result;
    for (const QJsonValue &messageValue: messages) {
        const QJsonObject message = messageValue.toObject();
        result.emplace_back(parseOneMessage(message));
    }

    std::sort(result.begin(), result.end());
    return result;
}

Message::Counter parseCountMessagesResponse(const QJsonDocument &response) {
    CHECK(response.isObject(), "Response field not found");
    const QJsonObject root = response.object();
    CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
    const QJsonObject data = root.value("data").toObject();
    CHECK(data.contains("cnt_last") && data.value("cnt_last").isString(), "cnt_last field not found");
    return std::stoll(data.value("cnt_last").toString().toStdString());
}

KeyMessageResponse parseKeyMessageResponse(const QJsonDocument &response) {
    KeyMessageResponse result;
    CHECK(response.isObject(), "Response field not found");
    const QJsonObject root = response.object();
    CHECK(root.contains("data") && root.value("data").isObject(), "data field not found");
    const QJsonObject data = root.value("data").toObject();
    CHECK(data.contains("rsa_pub_key") && data.value("rsa_pub_key").isString(), "rsa_pub_key field not found");
    result.publicKey = data.value("rsa_pub_key").toString();
    CHECK(data.contains("addr") && data.value("addr").isString(), "addr field not found");
    result.addr = data.value("addr").toString();
    CHECK(data.contains("fee") && data.value("fee").isString(), "fee field not found");
    result.fee = std::stoull(data.value("fee").toString().toStdString());

    return result;
}
