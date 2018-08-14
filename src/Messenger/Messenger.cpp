#include "Messenger.h"

#include "check.h"
#include "SlotWrapper.h"
#include "Log.h"
#include "makeJsFunc.h"

#include "MessengerMessages.h"
#include "MessengerJavascript.h"

#include <functional>
using namespace std::placeholders;

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QCryptographicHash>

#include "dbstorage.h"

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
    , db(*DBStorage::instance())
    , javascriptWrapper(javascriptWrapper)
    , wssClient("wss://messenger.metahash.io")
{
    CHECK(connect(this, SIGNAL(timerEvent()), this, SLOT(onTimerEvent())), "not connect onTimerEvent");
    CHECK(connect(&wssClient, &WebSocketClient::messageReceived, this, &Messenger::onWssMessageReceived), "not connect wssClient");
    CHECK(connect(this, SIGNAL(startedEvent()), this, SLOT(onRun())), "not connect run");

    CHECK(connect(this, &Messenger::registerAddress, this, &Messenger::onRegisterAddress), "not connect onRegisterAddress");
    CHECK(connect(this, &Messenger::savePubkeyAddress, this, &Messenger::onSavePubkeyAddress), "not connect onGetPubkeyAddress");
    CHECK(connect(this, &Messenger::getPubkeyAddress, this, &Messenger::onGetPubkeyAddress), "not connect onGetPubkeyAddress");
    CHECK(connect(this, &Messenger::sendMessage, this, &Messenger::onSendMessage), "not connect onSendMessage");
    CHECK(connect(this, &Messenger::signedStrings, this, &Messenger::onSignedStrings), "not connect onSignedStrings");
    CHECK(connect(this, &Messenger::getLastMessage, this, &Messenger::onGetLastMessage), "not connect onSignedStrings");
    CHECK(connect(this, &Messenger::getSavedPos, this, &Messenger::onGetSavedPos), "not connect onGetSavedPos");
    CHECK(connect(this, &Messenger::getSavedsPos, this, &Messenger::onGetSavedsPos), "not connect onGetSavedsPos");
    CHECK(connect(this, &Messenger::savePos, this, &Messenger::onSavePos), "not connect onGetSavedPos");
    CHECK(connect(this, &Messenger::getCountMessages, this, &Messenger::onGetCountMessages), "not connect onGetCountMessages");
    CHECK(connect(this, &Messenger::getHistoryAddress, this, &Messenger::onGetHistoryAddress), "not connect onGetHistoryAddress");
    CHECK(connect(this, &Messenger::getHistoryAddressAddress, this, &Messenger::onGetHistoryAddressAddress), "not connect onGetHistoryAddressAddress");
    CHECK(connect(this, &Messenger::getHistoryAddressAddressCount, this, &Messenger::onGetHistoryAddressAddressCount), "not connect onGetHistoryAddressAddressCount");

    wssClient.start();
}

void Messenger::invokeCallback(size_t requestId, const TypedException &exception) {
    auto found = callbacks.find(requestId);
    CHECK(found != callbacks.end(), "Not found callback for request " + std::to_string(requestId));
    const ResponseCallbacks callback = found->second; // копируем
    callbacks.erase(found);
    callback(exception);
}

std::vector<QString> Messenger::getMonitoredAddresses() const {
    const QStringList res = db.getUsersList();
    std::vector<QString> result;
    for (const QString r: res) {
        result.emplace_back(r);
    }
    return result;
}

void Messenger::getMessagesFromAddressFromWss(const QString &fromAddress, Message::Counter from, Message::Counter to) {
    const QString pubkeyHex = db.getUserPublicKey(fromAddress);
    const QString signHex = getSignFromMethod(fromAddress, makeTextForGetMyMessagesRequest());
    const QString message = makeGetMyMessagesRequest(pubkeyHex, signHex, from, to, id.get());
    emit wssClient.sendMessage(message);
}

void Messenger::clearAddressesToMonitored() {
    emit wssClient.setHelloString(std::vector<QString>{});
}

void Messenger::addAddressToMonitored(const QString &address) {
    LOG << "Add address to monitored " << address;
    const QString pubkeyHex = db.getUserPublicKey(address);
    const QString signHex = getSignFromMethod(address, makeTextForMsgAppendKeyOnlineRequest());
    const QString message = makeAppendKeyOnlineRequest(pubkeyHex, signHex, id.get());
    emit wssClient.addHelloString(message);
    emit wssClient.sendMessage(message);
}

QString Messenger::getSignFromMethod(const QString &address, const QString &method) const {
    const QString jsonString = db.getUserSignatures(address);
    const QJsonDocument json = QJsonDocument::fromJson(jsonString.toUtf8());
    CHECK(json.isArray(), "Incorrect json " + jsonString.toStdString());
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
    LOG << "Monitored addresses: " << monitoredAddresses.size();
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
            const Message::Counter lastCnt = db.getMessageMaxCounter(address);
            LOG << "Defferred process message " << address << " " << lastCnt;
            emit javascriptWrapper.newMessegesSig(address, lastCnt);
        }
    }
END_SLOT_WRAPPER
}

void Messenger::processMessages(const QString &address, const std::vector<NewMessageResponse> &messages) {
    CHECK(!messages.empty(), "Empty messages");
    const Message::Counter currConfirmedCounter = db.getMessageMaxConfirmedCounter(address);
    const Message::Counter minCounterInServer = messages.front().counter;
    const Message::Counter maxCounterInServer = messages.back().counter;

    for (const NewMessageResponse &m: messages) {
        const QString hashMessage = createHashMessage(m.data);
        if (m.isInput) {
            LOG << "Add message " << address << " " << m.collocutor << " " << m.counter;
            db.addMessage(address, m.collocutor, m.data, m.timestamp, m.counter, m.isInput, true, true, hashMessage, m.fee);
            const Message::Counter savedPos = db.getLastReadCounterForUserContact(address, m.collocutor);
            if (savedPos == -1) {
                db.setLastReadCounterForUserContact(address, m.collocutor, -1); // Это нужно, чтобы в базе данных отпечаталась связь между отправителем и получателем
            }
        } else {
            const auto id = db.findFirstNotConfirmedMessageWithHash(hashMessage); // TODO username
            if (id != -1) {
                db.updateMessage(id, m.counter, true);
                // Потом запросить сообщение по предыдущему counter output-а, если он изменился и такого номера еще нет, и установить deferrer
            } else {
                // Если сообщение не нашлось, поискать просто по хэшу. Если и оно не нашлось, то вставить
                db.addMessage(address, m.collocutor, m.data, m.timestamp, m.counter, m.isInput, false, true, hashMessage, m.fee);
            }
        }
    }

    if (minCounterInServer > currConfirmedCounter + 1) {
        LOG << "Deffer message " << address << " " << minCounterInServer << " " << currConfirmedCounter << " " << maxCounterInServer;
        deferredMessages[address].setDeferred(2s);
        getMessagesFromAddressFromWss(address, currConfirmedCounter + 1, minCounterInServer);
    } else {
        if (!deferredMessages[address].isDeferred()) {
            emit javascriptWrapper.newMessegesSig(address, maxCounterInServer);
        } else {
            LOG << "Deffer message2 " << address << " " << minCounterInServer << " " << currConfirmedCounter << " " << maxCounterInServer;
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
            invokeCallback(responseType.id, TypedException(TypeErrors::MESSENGER_ERROR, responseType.error.toStdString()));
        }
        return;
    }

    if (responseType.method == METHOD::APPEND_KEY_TO_ADDR) {
        invokeCallback(responseType.id, TypedException());
    } else if (responseType.method == METHOD::COUNT_MESSAGES) {
        const Message::Counter currCounter = db.getMessageMaxConfirmedCounter(responseType.address);
        const Message::Counter messagesInServer = parseCountMessagesResponse(messageJson);
        if (currCounter < messagesInServer) {
            LOG << "Read missing messages " << responseType.address << " " << currCounter + 1 << " " << messagesInServer;
            getMessagesFromAddressFromWss(responseType.address, currCounter + 1, messagesInServer);
        } else {
            LOG << "Count messages " << responseType.address << " " << currCounter << " " << messagesInServer;
        }
    } else if (responseType.method == METHOD::GET_KEY_BY_ADDR) {
        const KeyMessageResponse publicKeyResult = parseKeyMessageResponse(messageJson);
        LOG << "Save pubkey " << publicKeyResult.addr << " " << publicKeyResult.publicKey;
        db.setContactPublicKey(publicKeyResult.addr, publicKeyResult.publicKey);
        invokeCallback(responseType.id, TypedException());
    } else if (responseType.method == METHOD::NEW_MSG) {
        const NewMessageResponse messages = parseNewMessageResponse(messageJson);
        LOG << "New msg " << responseType.address << " " << messages.collocutor << " " << messages.counter;
        processMessages(responseType.address, {messages});
    } else if (responseType.method == METHOD::NEW_MSGS) {
        const std::vector<NewMessageResponse> messages = parseNewMessagesResponse(messageJson);
        LOG << "New msgs " << responseType.address << " " << messages.size();
        processMessages(responseType.address, messages);
    } else if (responseType.method == METHOD::SEND_TO_ADDR) {
        LOG << "Send to addr ok " << responseType.address;
        invokeCallback(responseType.id, TypedException());
    } else {
        throwErr("Incorrect response type");
    }
END_SLOT_WRAPPER
}

void Messenger::onRegisterAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const RegisterAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    const QString currPubkey = db.getUserPublicKey(address);
    const bool isNew = currPubkey.isEmpty();
    if (!isNew && !isForcibly) {
        callback(isNew, TypedException());
        return;
    }
    const size_t idRequest = id.get();
    const QString message = makeRegisterRequest(rsaPubkeyHex, pubkeyAddressHex, signHex, fee, idRequest);
    const auto callbackWrap = [this, callback, isNew, address, pubkeyAddressHex, isForcibly](const TypedException &exception) {
        if (!exception.isSet() || isForcibly) { // TODO убрать isForcibly
            LOG << "Set user pubkey " << address << " " << pubkeyAddressHex;
            db.setUserPublicKey(address, pubkeyAddressHex);
        }
        callback(isNew, exception);
    };
    callbacks[idRequest] = callbackWrap;
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onSignedStrings(const QString &address, const std::vector<QString> &signedHexs, const SignedStringsCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
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
        LOG << "Set user signature " << arr.size();
        db.setUserSignatures(address, arr);
        addAddressToMonitored(address);
    });

    emit javascriptWrapper.callbackCall(std::bind(callback, exception));
END_SLOT_WRAPPER
}

void Messenger::onSavePubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex, const SavePubkeyCallback &callback) {
BEGIN_SLOT_WRAPPER
    const QString currSign = db.getUserSignatures(address);
    const bool isNew = currSign.isEmpty();
    if (!isNew && !isForcibly) {
        callback(isNew, TypedException());
        return;
    }
    const size_t idRequest = id.get();
    const QString message = makeGetPubkeyRequest(address, pubkeyHex, signHex, idRequest);
    callbacks[idRequest] = std::bind(callback, isNew, _1);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onGetPubkeyAddress(const QString &address, const GetPubkeyAddress &callback) {
BEGIN_SLOT_WRAPPER
    QString pubkey = "";
    const TypedException exception = apiVrapper2([&, this] {
        pubkey = db.getContactrPublicKey(address);
        LOG << "Publickey found " << address << " " << pubkey;
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, pubkey, exception));
END_SLOT_WRAPPER
}

void Messenger::onSendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    const QString hashMessage = createHashMessage(dataHex);
    const Message::Counter lastCnt = db.getMessageMaxCounter(thisAddress);
    db.addMessage(thisAddress, toAddress, encryptedDataHex, timestamp, lastCnt + 1, false, true, false, hashMessage, fee);
    const size_t idRequest = id.get();
    const QString message = makeSendMessageRequest(toAddress, dataHex, pubkeyHex, signHex, fee, timestamp, idRequest);
    callbacks[idRequest] = callback;
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onGetSavedPos(const QString &address, const QString &collocutor, const GetSavedPosCallback &callback) {
BEGIN_SLOT_WRAPPER
    Message::Counter lastCounter;
    const TypedException exception = apiVrapper2([&, this] {
        lastCounter = db.getLastReadCounterForUserContact(address, collocutor);
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, lastCounter, exception));
END_SLOT_WRAPPER
}

void Messenger::onGetSavedsPos(const QString &address, const GetSavedsPosCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<std::pair<QString, Message::Counter>> pos;
    const TypedException exception = apiVrapper2([&, this] {
        const std::list<std::pair<QString, Message::Counter>> result = db.getLastReadCountersForUser(address);
        std::copy(result.cbegin(), result.cend(), std::back_inserter(pos));
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, pos, exception));
END_SLOT_WRAPPER
}

void Messenger::onSavePos(const QString &address, const QString &collocutor, Message::Counter pos, const SavePosCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        db.setLastReadCounterForUserContact(address, collocutor, pos);
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, exception));
END_SLOT_WRAPPER
}

void Messenger::onGetLastMessage(const QString &address, const GetSavedPosCallback &callback) {
BEGIN_SLOT_WRAPPER
    Message::Counter lastCounter;
    const TypedException exception = apiVrapper2([&, this] {
        lastCounter = db.getMessageMaxCounter(address);
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, lastCounter, exception));
END_SLOT_WRAPPER
}

void Messenger::onGetCountMessages(const QString &address, const QString &collocutor, Message::Counter from, const GetCountMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    Message::Counter lastCounter = 0;
    const TypedException exception = apiVrapper2([&, this] {
        lastCounter = db.getMessagesCountForUserAndDest(address, collocutor, from);
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, lastCounter, exception));
END_SLOT_WRAPPER
}

void Messenger::onGetHistoryAddress(QString address, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Message> messages;
    const TypedException exception = apiVrapper2([&, this] {
        const std::list<Message> result = db.getMessagesForUser(address, from, to);
        std::copy(result.begin(), result.end(), std::back_inserter(messages));
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, messages, exception));
END_SLOT_WRAPPER
}

void Messenger::onGetHistoryAddressAddress(QString address, QString collocutor, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Message> messages;
    const TypedException exception = apiVrapper2([&, this] {
        const std::list<Message> result = db.getMessagesForUserAndDest(address, collocutor, from, to);
        std::copy(result.begin(), result.end(), std::back_inserter(messages));
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, messages, exception));
END_SLOT_WRAPPER
}

void Messenger::onGetHistoryAddressAddressCount(QString address, QString collocutor, Message::Counter count, Message::Counter to, const GetMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Message> messages;
    const TypedException exception = apiVrapper2([&, this] {
        const std::list<Message> result = db.getMessagesForUserAndDestNum(address, collocutor, to, count);
        std::copy(result.begin(), result.end(), std::back_inserter(messages));
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, messages, exception));
END_SLOT_WRAPPER
}
