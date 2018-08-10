#include "MessengerMessages.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "check.h"

const static QString MSG_GET_MY_REQUEST = "msg_get_my";
const static QString MSG_GET_CHANNEL_REQUEST = "msg_get_channel";
const static QString MSG_GET_CHANNELS_REQUEST = "msg_get_channels";
const static QString MSG_APPEND_KEY_ONLINE_REQUEST = "msg_append_key_online";

const static QString APPEND_KEY_TO_ADDR_RESPONSE = "";
const static QString GET_KEY_BY_ADDR_RESPONSE = "";
const static QString SEND_TO_ADDR_RESPONSE = "";
const static QString NEW_MSGS_RESPONSE = "";
const static QString NEW_MSG_RESPONSE = "";
const static QString COUNT_MESSAGES_RESPONSE = "";

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
    json.insert("id", QString::fromStdString(std::to_string(id)));
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
    json.insert("id", QString::fromStdString(std::to_string(id)));
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
    json.insert("id", QString::fromStdString(std::to_string(id)));
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
    json.insert("id", QString::fromStdString(std::to_string(id)));
    QJsonObject params;
    params.insert("cnt", QString::fromStdString(std::to_string(from)));
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

QString makeAppendKeyOnlineRequest(const QString &pubkeyHex, const QString &signHex, size_t id) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", MSG_APPEND_KEY_ONLINE_REQUEST);
    json.insert("id", QString::fromStdString(std::to_string(id)));
    QJsonObject params;
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

ResponseType getMethodAndAddressResponse(const QJsonDocument &response) {
    // Может не быть адреса или метода, если ошибка
    ResponseType result;
    const QString type = "";
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

NewMessageResponse parseNewMessageResponse(const QJsonDocument &response) {
    NewMessageResponse result;
    return result;
}

std::vector<NewMessageResponse> parseNewMessagesResponse(const QJsonDocument &response) {
    std::vector<NewMessageResponse> result;

    std::sort(result.begin(), result.end());
    return result;
}

Message::Counter parseCountMessagesResponse(const QJsonDocument &response) {
    return 0;
}

std::pair<QString, QString> parseKeyMessageResponse(const QJsonDocument &response) {
    const QString address = "";
    const QString publicKey = "";
    return std::make_pair(address, publicKey);
}
