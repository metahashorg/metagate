#include "Messenger.h"

#include "check.h"
#include "SlotWrapper.h"
#include "Log.h"

#include "MessengerMessages.h"

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QCryptographicHash>

#include "MessengerJavascript.h"

static QString createHashMessage(const QString &message) {
    return QString(QCryptographicHash::hash(message.toUtf8(), QCryptographicHash::Sha512).toHex());
}

std::vector<QString> Messenger::stringsForSign() {
    return {makeTextForGetMyMessagesRequest(), makeTextForGetChannelRequest(), makeTextForGetChannelsRequest(), makeTextForMsgAppendKeyOnlineRequest()};
}

QString Messenger::makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee) {
    return ::makeTextForSignRegisterRequest(address, rsaPubkeyHex, fee);
}

QString Messenger::makeTextForGetPubkeyRequest(const QString &address) {
    return ::makeTextForGetPubkeyRequest(address);
}

QString Messenger::makeTextForSendMessageRequest(const QString &address, const QString &dataHex, uint64_t fee) {
    return ::makeTextForSendMessageRequest(address, dataHex, fee);
}

Messenger::Messenger(MessengerJavascript &javascriptWrapper, QObject *parent)
    : TimerClass(1s, parent)
    , javascriptWrapper(javascriptWrapper)
    , wssClient("wss.wss.com")
{
    CHECK(connect(this, SIGNAL(timerEvent()), this, SLOT(onTimerEvent())), "not connect onTimerEvent");
    CHECK(connect(&wssClient, &WebSocketClient::messageReceived, this, &Messenger::onWssMessageReceived), "not connect wssClient");
    CHECK(connect(this, SIGNAL(startedEvent()), this, SLOT(onRun())), "not connect run");

    CHECK(connect(this, &Messenger::registerAddress, this, &Messenger::onRegisterAddress), "not connect onRegisterAddress");
    CHECK(connect(this, &Messenger::getPubkeyAddress, this, &Messenger::onGetPubkeyAddress), "not connect onGetPubkeyAddress");
    CHECK(connect(this, &Messenger::sendMessage, this, &Messenger::onSendMessage), "not connect onSendMessage");
    CHECK(connect(this, &Messenger::signedStrings, this, &Messenger::onSignedStrings), "not connect onSignedStrings");
    CHECK(connect(this, &Messenger::getLastMessage, this, &Messenger::onGetLastMessage), "not connect onSignedStrings");
    CHECK(connect(this, &Messenger::getSavedPos, this, &Messenger::onGetSavedPos), "not connect onGetSavedPos");
    CHECK(connect(this, &Messenger::savePos, this, &Messenger::onSavePos), "not connect onGetSavedPos");
    CHECK(connect(this, &Messenger::getHistoryAddress, this, &Messenger::onGetHistoryAddress), "not connect onGetHistoryAddress");
    CHECK(connect(this, &Messenger::getHistoryAddressAddress, this, &Messenger::onGetHistoryAddressAddress), "not connect onGetHistoryAddressAddress");
    CHECK(connect(this, &Messenger::getHistoryAddressAddressCount, this, &Messenger::onGetHistoryAddressAddressCount), "not connect onGetHistoryAddressAddressCount");

    wssClient.start();
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

void Messenger::onGetLastMessage(const QString &address) {
BEGIN_SLOT_WRAPPER
    // Получить counter
    Message::Counter lastCounter = 0;
    emit javascriptWrapper.lastMessageSig(address, lastCounter);
END_SLOT_WRAPPER
}

void Messenger::onGetSavedPos(const QString &address) {
BEGIN_SLOT_WRAPPER
    // Получить counter
    Message::Counter lastCounter = 0;
    emit javascriptWrapper.savedPosSig(address, lastCounter);
END_SLOT_WRAPPER
}

void Messenger::onSavePos(const QString &address, Message::Counter pos) {
BEGIN_SLOT_WRAPPER
    // Сохранить позицию
    emit javascriptWrapper.storePosSig(address);
END_SLOT_WRAPPER
}

QString Messenger::getSignFromMethod(const QString &address, const QString &method) const {
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
    const std::vector<QString> monitoredAddresses = getMonitoredAddresses();
    clearAddressesToMonitored();
    for (const QString &address: monitoredAddresses) {
        addAddressToMonitored(address);
    }
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
            const Message::Counter lastCnt = 0;
            emit javascriptWrapper.newMessegesSig(address, lastCnt);
        }
    }
END_SLOT_WRAPPER
}

void Messenger::processMessages(const QString &address, const std::vector<NewMessageResponse> &messages) {
    CHECK(!messages.empty(), "Empty messages");
    // Запросить counter из bd
    const Message::Counter currCounter = 0;
    const Message::Counter minCounterInServer = messages.front().counter;
    const Message::Counter maxCounterInServer = messages.back().counter;

    for (const NewMessageResponse &m: messages) {
        if (m.isInput) {
            // сохранить сообщение в бд
        } else {
            // Вычислить хэш сообщения, найти сообщение в bd минимальное по номеру, которое не подтвержденное, заменить у него counter. Если сообщение не нашлось, поискать просто по хэшу. Если и оно не нашлось, то вставить
            // Потом запросить сообщение по предыдущему counter output-а, если он изменился и такого номера еще нет, и установить deferrer
        }
    }

    if (minCounterInServer > currCounter + 1) {
        deferredMessages[address].setDeferred(2s);
        getMessagesFromAddressFromWss(address, currCounter + 1, minCounterInServer);
    } else {
        if (!deferredMessages[address].isDeferred()) {
            emit javascriptWrapper.newMessegesSig(address, maxCounterInServer);
        }
    }
}

void Messenger::onWssMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument messageJson = QJsonDocument::fromJson(message.toUtf8());
    const ResponseType responseType = getMethodAndAddressResponse(messageJson);

    if (responseType.isError) {
        LOG << "Messenger response error " << responseType.method << " " << responseType.address << " " << responseType.error;
        if (responseType.method != METHOD::NOT_SET && !responseType.address.isEmpty() && !responseType.address.isNull()) {
            emit javascriptWrapper.operationUnluckySig(responseType.method, responseType.address, responseType.error);
        }
        return;
    }

    if (responseType.method == METHOD::APPEND_KEY_TO_ADDR) {
        emit javascriptWrapper.addressAppendToMessengerSig(responseType.address);
    } else if (responseType.method == METHOD::COUNT_MESSAGES) {
        // Получить из бд количество сообщений для адреса
        const Message::Counter currCounter = 0;
        const Message::Counter messagesInServer = parseCountMessagesResponse(messageJson);
        if (currCounter < messagesInServer) {
            getMessagesFromAddressFromWss(responseType.address, currCounter + 1, messagesInServer); // TODO уточнить, to - это включительно или нет
        }
    } else if (responseType.method == METHOD::GET_KEY_BY_ADDR) {
        const auto publicKeyPair = parseKeyMessageResponse(messageJson);
        const QString &address = publicKeyPair.first;
        const QString &pkey = publicKeyPair.second;
        // Сохранить в базу данных соответствие
        emit javascriptWrapper.publicKeyCollocutorGettedSig(responseType.address, address);
    } else if (responseType.method == METHOD::NEW_MSG) {
        const NewMessageResponse messages = parseNewMessageResponse(messageJson);
        processMessages(responseType.address, {messages});
    } else if (responseType.method == METHOD::NEW_MSGS) {
        const std::vector<NewMessageResponse> messages = parseNewMessagesResponse(messageJson);
        processMessages(responseType.address, messages);
    } else if (responseType.method == METHOD::SEND_TO_ADDR) {
        // Получить адрес получателя
        const QString collocutor = "";
        emit javascriptWrapper.messageSendedSig(responseType.address, collocutor);
    } else {
        throwErr("Incorrect response type");
    }
END_SLOT_WRAPPER
}

void Messenger::onRegisterAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee) {
BEGIN_SLOT_WRAPPER
    // Проверить в базе, если пользователь уже зарегистрирован, то больше не регестрировать
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

void Messenger::onSendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex) {
BEGIN_SLOT_WRAPPER
    // Вычислить хэш dataHex, Поместить сообщение encryptedDataHex в базу данных под максимальным номером
    const QString message = makeSendMessageRequest(toAddress, dataHex, pubkeyHex, signHex, fee, timestamp);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::getMessagesFromAddressFromWss(const QString &fromAddress, Message::Counter from, Message::Counter to) {
    // Получаем sign и pubkey для данного типа сообщений из базы
    const QString pubkeyHex = "";
    const QString signHex = getSignFromMethod(fromAddress, makeTextForGetMyMessagesRequest());
    const QString message = makeGetMyMessagesRequest(pubkeyHex, signHex, from, to);
    emit wssClient.sendMessage(message);
}

void Messenger::clearAddressesToMonitored() {
    emit wssClient.setHelloString(std::vector<QString>{});
}

void Messenger::addAddressToMonitored(const QString &address) {
    // Получаем sign для данного типа сообщений из базы
    const QString pubkeyHex = "";
    const QString signHex = getSignFromMethod(address, makeTextForMsgAppendKeyOnlineRequest());
    const QString message = makeAppendKeyOnlineRequest(pubkeyHex, signHex);
    emit wssClient.addHelloString(message);
    emit wssClient.sendMessage(message);
}

void Messenger::onGetHistoryAddress(QString address, Message::Counter from, Message::Counter to) {
BEGIN_SLOT_WRAPPER
    // Получить сообщения
    std::vector<Message> messages;
    emit javascriptWrapper.getHistoryAddressSig(address, messages);
END_SLOT_WRAPPER
}

void Messenger::onGetHistoryAddressAddress(QString address, QString collocutor, Message::Counter from, Message::Counter to) {
BEGIN_SLOT_WRAPPER
    // Получить сообщения
    std::vector<Message> messages;
    emit javascriptWrapper.getHistoryAddressAddressSig(address, messages);
END_SLOT_WRAPPER
}

void Messenger::onGetHistoryAddressAddressCount(QString address, QString collocutor, Message::Counter count, Message::Counter to) {
BEGIN_SLOT_WRAPPER
    // Получить сообщения
    std::vector<Message> messages;
    emit javascriptWrapper.getHistoryAddressAddressCountSig(address, messages);
END_SLOT_WRAPPER
}
