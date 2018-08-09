#include "Messenger.h"

#include "check.h"
#include "SlotWrapper.h"
#include "Log.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

const static QString APPEND_KEY_TO_ADDR_RESPONSE = "";
const static QString GET_KEY_BY_ADDR_RESPONSE = "";
const static QString SEND_TO_ADDR_RESPONSE = "";
const static QString NEW_MSGS_RESPONSE = "";
const static QString NEW_MSG_RESPONSE = "";
const static QString COUNT_MESSAGES_RESPONSE = "";

const static QString MSG_GET_MY_REQUEST = "msg_get_my";
const static QString MSG_GET_CHANNEL = "msg_get_channel";
const static QString MSG_GET_CHANNELS = "msg_get_channels";
const static QString MSG_APPEND_KEY_ONLINE = "msg_append_key_online";

std::vector<QString> Messenger::stringsForSign() {
    return {MSG_GET_MY_REQUEST, MSG_GET_CHANNEL, MSG_GET_CHANNELS, MSG_APPEND_KEY_ONLINE};
}

Messenger::Messenger(QObject *parent)
    : TimerClass(1s, parent)
    , wssClient("wss.wss.com")
{
    CHECK(connect(this, SIGNAL(timerEvent()), this, SLOT(onTimerEvent())), "not connect onTimerEvent");
    CHECK(connect(&wssClient, &WebSocketClient::messageReceived, this, &Messenger::onWssMessageReceived), "not connect wssClient");
    CHECK(connect(this, SIGNAL(startedEvent()), this, SLOT(onRun())), "not connect run");

    CHECK(connect(this, &Messenger::registerAddress, this, &Messenger::onRegisterAddress), "not connect onRegisterAddress");
    CHECK(connect(this, &Messenger::getPubkeyAddress, this, &Messenger::onGetPubkeyAddress), "not connect onGetPubkeyAddress");
    CHECK(connect(this, &Messenger::sendMessage, this, &Messenger::onSendMessage), "not connect onSendMessage");
    CHECK(connect(this, &Messenger::signedStrings, this, &Messenger::onSignedStrings), "not connect onSignedStrings");

    wssClient.start();
}

static QString makeRegisterRequest(const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", "msg_append_key_to_addr");
    QJsonObject params;
    params.insert("fee", QString::fromStdString(std::to_string(fee)));
    params.insert("rsa_pub_key", rsaPubkeyHex);
    params.insert("pubkey", pubkeyAddressHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

static QString makeGetPubkeyRequest(const QString &address, const QString &pubkeyHex, const QString &signHex) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", "msg_get_key_by_addr");
    QJsonObject params;
    params.insert("addr", address);
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

static QString makeSendMessageRequest(const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", "msg_send_to_addr");
    QJsonObject params;
    params.insert("to", toAddress);
    params.insert("data", dataHex);
    params.insert("fee", QString::fromStdString(std::to_string(fee)));
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

static QString makeGetMyMessagesRequest(const QString &pubkeyHex, const QString &signHex, Messenger::Counter from, Messenger::Counter to) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", "msg_get_my");
    QJsonObject params;
    params.insert("cnt", QString::fromStdString(std::to_string(from)));
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

static QString makeAppendKeyOnlineRequest(const QString &pubkeyHex, const QString &signHex) {
    QJsonObject json;
    json.insert("jsonrpc", "2.0");
    json.insert("method", "msg_append_key_online");
    QJsonObject params;
    params.insert("pubkey", pubkeyHex);
    params.insert("sign", signHex);
    json.insert("params", params);
    return QJsonDocument(json).toJson(QJsonDocument::Compact);
}

namespace {
enum class METHOD: int {
    APPEND_KEY_TO_ADDR = 0, GET_KEY_BY_ADDR = 1, SEND_TO_ADDR = 2, NEW_MSGS = 3, NEW_MSG = 4, COUNT_MESSAGES = 5
};

struct ResponseType {
    METHOD method;
    QString address;
    bool isError;
    QString error;
};

ResponseType getMethodAndAddressResponse(const QJsonDocument &response) {
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
}

static Messenger::NewMessageResponse parseNewMessageResponse(const QJsonDocument &response) {
    Messenger::NewMessageResponse result;
    return result;
}

static std::vector<Messenger::NewMessageResponse> parseNewMessagesResponse(const QJsonDocument &response) {
    std::vector<Messenger::NewMessageResponse> result;

    std::sort(result.begin(), result.end());
    return result;
}

static Messenger::Counter parseCountMessagesResponse(const QJsonDocument &response) {
    return 0;
}

static std::pair<QString, QString> parseKeyMessageResponse(const QJsonDocument &response) {
    const QString address = "";
    const QString publicKey = "";
    return std::make_pair(address, publicKey);
}

std::vector<QString> Messenger::getMonitoredAddresses() const {
    // Взять из базы данных все адреса
    std::vector<QString> result;
    return result;
}

void Messenger::onSignedStrings(const std::vector<QString> &signedHexs) {
BEGIN_SLOT_WRAPPER
    const std::vector<QString> keys = stringsForSign();
    CHECK(keys.size() == signedHexs.size(), "Incorrect signed strings");

    QJsonArray arrJson;
    for (size_t i = 0; i < keys.size(); i++) {
        const QString &key = keys[i];
        const QString &value = signedHexs[i];

        QJsonObject obj;
        obj.insert("key", key);
        obj.insert("value", value);
        arrJson.push_back(obj);
    }

    const QString arr = QJsonDocument(arrJson).toJson(QJsonDocument::Compact);
    // Сохранить arr в бд
END_SLOT_WRAPPER
}

QString Messenger::getPublicKeyFromMethod(const QString &address, const QString &method) const {
    // Взять json из бд
    const QString jsonString = "";
    const QJsonDocument json = QJsonDocument::fromJson(jsonString.toUtf8());
    CHECK(json.isArray(), "Incorrect json");
    const QJsonArray &array = json.array();
    for (const QJsonValue &val: array) {
        CHECK(val.isObject(), "Incorrect json");
        const QJsonObject v = val.toObject();
        CHECK(v.contains("key") && v.value("key").isString(), "Incorrect json: key field not found");
        const QString key = v.value("key").toString();
        CHECK(v.contains("value") && v.value("value").isString(), "Incorrect json: value field not found");
        const QString value = v.value("value").toString();

        if (key == method) {
            return value;
        }
    }
    throwErr(("Not found signed method " + method + " in address " + address).toStdString());
}

void Messenger::onRun() {
BEGIN_SLOT_WRAPPER
END_SLOT_WRAPPER
}

void Messenger::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    for (auto &pairDeferred: deferredMessages) {
        const QString &address = pairDeferred.first;
        DeferredMessage &deferred = pairDeferred.second;
        if (deferred.check()) {
            deferred.resetDeferred();
            // Взять последнее значение из бд
            const Counter lastCnt = 0;
            // Послать сообщение в js
        }
    }
END_SLOT_WRAPPER
}

void Messenger::processMessages(const QString &address, const std::vector<NewMessageResponse> &messages) {
    CHECK(!messages.empty(), "Empty messages");
    // Запросить counter из bd
    const Counter currCounter = 0;
    const Counter minCounterInServer = messages.front().counter;
    const Counter maxCounterInServer = messages.back().counter;

    for (const NewMessageResponse &m: messages) {
        if (m.isInput) {
            // сохранить сообщение в бд
        } else {
            // Вычислить хэш сообщения, найти сообщение в bd, заменить у него counter
        }
    }

    if (minCounterInServer > currCounter + 1) {
        deferredMessages[address].setDeferred(2s);
    } else {
        if (!deferredMessages[address].isDeferred()) {
            // Сказать javascript-у, что есть новые сообщения maxCounterInServer
        }
    }
}

void Messenger::onWssMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument messageJson = QJsonDocument::fromJson(message.toUtf8());
    const ResponseType responseType = getMethodAndAddressResponse(messageJson);

    if (responseType.isError) {
        LOG << "Messenger response error " << responseType.error;
        // Отправить ошибку в javascript
        return;
    }

    if (responseType.method == METHOD::APPEND_KEY_TO_ADDR) {
        // Вызвать javascript, что все ок
    } else if (responseType.method == METHOD::COUNT_MESSAGES) {
        // Получить из бд количество сообщений для адреса
        const Counter currCounter = 0;
        const Counter messagesInServer = parseCountMessagesResponse(messageJson);
        if (currCounter == messagesInServer) {
            // Нету новых сообщений. Выходим
            return;
        }
        getMessagesFromAddressFromWss(responseType.address, currCounter + 1, messagesInServer); // TODO уточнить, to - это включительно или нет
    } else if (responseType.method == METHOD::GET_KEY_BY_ADDR) {
        const auto publicKeyPair = parseKeyMessageResponse(messageJson);
        const QString &address = publicKeyPair.first;
        const QString &pkey = publicKeyPair.second;
        // Сохранить в базу данных соответствие
        // Вызвать javascript, что все ок
    } else if (responseType.method == METHOD::NEW_MSG) {
        const NewMessageResponse messages = parseNewMessageResponse(messageJson);
        processMessages(responseType.address, {messages});
    } else if (responseType.method == METHOD::NEW_MSGS) {
        const std::vector<NewMessageResponse> messages = parseNewMessagesResponse(messageJson);
        processMessages(responseType.address, messages);
    } else if (responseType.method == METHOD::SEND_TO_ADDR) {
        // Отправить javascript, что все ok
    } else {
        throwErr("Incorrect response type");
    }
END_SLOT_WRAPPER
}

void Messenger::onRegisterAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee) {
BEGIN_SLOT_WRAPPER
    // Проверить в базе, если пользователь уже зарегистрирован, то больше не регестрировать. Также добавить pubkeyAddressHex в базу
    const QString message = makeRegisterRequest(rsaPubkeyHex, pubkeyAddressHex, signHex, fee);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onGetPubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex) {
BEGIN_SLOT_WRAPPER
    // Проверить, есть ли нужных ключ в базе
    const QString message = makeGetPubkeyRequest(address, pubkeyHex, signHex);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onSendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, const QString &encryptedDataHex) {
BEGIN_SLOT_WRAPPER
    // Вычислить хэш dataHex, Поместить сообщение encryptedDataHex в базу данных под максимальным номером
    const QString message = makeSendMessageRequest(toAddress, dataHex, pubkeyHex, signHex, fee);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::getMessagesFromAddressFromWss(const QString &fromAddress, Counter from, Counter to) {
    // Получаем sign и pubkey для данного типа сообщений из базы
    const QString pubkeyHex = "";
    const QString signHex = "";
    const QString message = makeGetMyMessagesRequest(pubkeyHex, signHex, from, to);
    emit wssClient.sendMessage(message);
}

void Messenger::clearAddressesToMonitored() {
    emit wssClient.setHelloString(std::vector<QString>{});
}

void Messenger::addAddressToMonitored(const QString &address) {
    // Получаем sign для данного типа сообщений из базы
    const QString pubkeyHex = "";
    const QString signHex = "";
    const QString message = makeAppendKeyOnlineRequest(pubkeyHex, signHex);
    emit wssClient.addHelloString(message);
    emit wssClient.sendMessage(message);
}
