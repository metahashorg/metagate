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

#include "messengerdbstorage.h"

static QString createHashMessage(const QString &message) {
    return QString(QCryptographicHash::hash(message.toUtf8(), QCryptographicHash::Sha512).toHex());
}

std::vector<QString> Messenger::stringsForSign() {
    return {makeTextForGetMyMessagesRequest(), makeTextForGetChannelRequest(), makeTextForGetChannelsRequest(), makeTextForMsgAppendKeyOnlineRequest(), makeTextForGetMyChannelsRequest()};
}

QString Messenger::makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee) {
    return ::makeTextForSignRegisterRequest(address, rsaPubkeyHex, fee);
}

QString Messenger::makeTextForGetPubkeyRequest(const QString &address) {
    return ::makeTextForGetPubkeyRequest(address);
}

QString Messenger::makeTextForSendMessageRequest(const QString &address, const QString &dataHex, uint64_t fee, uint64_t timestamp) {
    return ::makeTextForSendMessageRequest(address, dataHex, fee, timestamp);
}

QString Messenger::makeTextForChannelCreateRequest(const QString &title, const QString titleSha, uint64_t fee) {
    return ::makeTextForChannelCreateRequest(title, titleSha, fee);
}

QString Messenger::makeTextForChannelAddWriterRequest(const QString &titleSha, const QString &address) {
    return ::makeTextForChannelAddWriterRequest(titleSha, address);
}

QString Messenger::makeTextForChannelDelWriterRequest(const QString &titleSha, const QString &address) {
    return ::makeTextForChannelDelWriterRequest(titleSha, address);
}

QString Messenger::makeTextForSendToChannelRequest(const QString &titleSha, const QString &text, uint64_t fee, uint64_t timestamp) {
    return ::makeTextForSendToChannelRequest(titleSha, text, fee, timestamp);
}

Messenger::Messenger(MessengerJavascript &javascriptWrapper, MessengerDBStorage &db, QObject *parent)
    : TimerClass(1s, parent)
    , db(db)
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
    CHECK(requestId != size_t(-1), "Incorrect request id");
    const auto found = callbacks.find(requestId);
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
    CHECK_TYPED(!pubkeyHex.isEmpty(), TypeErrors::INCOMPLETE_USER_INFO, "user pubkey not found " + fromAddress.toStdString());
    const QString signHex = getSignFromMethod(fromAddress, makeTextForGetMyMessagesRequest());
    const QString message = makeGetMyMessagesRequest(pubkeyHex, signHex, from, to, id.get());
    emit wssClient.sendMessage(message);
}

void Messenger::getMessagesFromChannelFromWss(const QString &fromAddress, const QString &channelSha, Message::Counter from, Message::Counter to) {
    const QString pubkeyHex = db.getUserPublicKey(fromAddress);
    CHECK_TYPED(!pubkeyHex.isEmpty(), TypeErrors::INCOMPLETE_USER_INFO, "user pubkey not found " + fromAddress.toStdString());
    const QString signHex = getSignFromMethod(fromAddress, makeTextForGetChannelRequest());
    const QString message = makeGetChannelRequest(channelSha, from, to, pubkeyHex, signHex, id.get());
    emit wssClient.sendMessage(message);
}

void Messenger::clearAddressesToMonitored() {
    emit wssClient.setHelloString(std::vector<QString>{});
}

void Messenger::addAddressToMonitored(const QString &address) {
    LOG << "Add address to monitored " << address;
    const QString pubkeyHex = db.getUserPublicKey(address);
    CHECK_TYPED(!pubkeyHex.isEmpty(), TypeErrors::INCOMPLETE_USER_INFO, "user pubkey not found " + address.toStdString());
    const QString signHex = getSignFromMethod(address, makeTextForMsgAppendKeyOnlineRequest());
    const QString message = makeAppendKeyOnlineRequest(pubkeyHex, signHex, id.get());
    emit wssClient.addHelloString(message);
    emit wssClient.sendMessage(message);

    const QString signHexChannels = getSignFromMethod(address, makeTextForGetMyChannelsRequest());
    const QString messageGetMyChannels = makeGetMyChannelsRequest(pubkeyHex, signHexChannels, id.get());
    emit wssClient.addHelloString(messageGetMyChannels);
    emit wssClient.sendMessage(messageGetMyChannels);
}

void Messenger::processMyChannels(const QString &address, const std::vector<ChannelInfo> &channels) {
    // Сбросить флаг isVisited у всех channel-ей в таблице
    // Пройтись по всему массиву, добавить новую инфу с флагом isVisited или установить флаг isVisited, если инфа существует
    // У всех записей, где флаг isVisited не установлен, поставить isWriter = false
    // Для новых каналов поставить счетчик прочитанных в -1
    // Сбросить флаг isVisited у всех channel-ей в таблице
    for (const ChannelInfo &channel: channels) {
        // Взять индекс последнего сообщения из бд
        const Message::Counter counter = -1;
        if (counter < channel.counter) {
            getMessagesFromChannelFromWss(address, channel.titleSha, counter + 1, channel.counter);
        }
    }
}

void Messenger::processAddOrDeleteInChannel(const QString &address, const ChannelInfo &channel, bool isAdd) {
    if (!isAdd) {
        // поставить метку, что канал удален
        // emit удалился новый канал
        return;
    }
    // занести новый канал
    // Получить counter из channel или запросить
    const Message::Counter cnt = -1;
    if (cnt != -1) {
        getMessagesFromChannelFromWss(address, channel.titleSha, 0, cnt);
    }
    // emit появился новый канал
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
    throwErrTyped(TypeErrors::INCOMPLETE_USER_INFO, ("Not found signed method " + method + " in address " + address).toStdString());
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
        const QString &address = pairDeferred.first.first;
        const QString &channel = pairDeferred.first.second;
        DeferredMessage &deferred = pairDeferred.second;
        if (deferred.check()) {
            deferred.resetDeferred();
            const Message::Counter lastCnt = db.getMessageMaxCounter(address);
            LOG << "Defferred process message " << address << " " << channel << " " << lastCnt;
            if (channel.isEmpty()) {
                emit javascriptWrapper.newMessegesSig(address, lastCnt);
            } else {
                // newMessageSigChannel
            }
        }
    }
END_SLOT_WRAPPER
}

void Messenger::processMessages(const QString &address, const std::vector<NewMessageResponse> &messages, bool isChannel) {
    CHECK(!messages.empty(), "Empty messages");
    const Message::Counter currConfirmedCounter = db.getMessageMaxConfirmedCounter(address);
    CHECK(std::is_sorted(messages.begin(), messages.end()), "Messages not sorted");
    const Message::Counter minCounterInServer = messages.front().counter;
    const Message::Counter maxCounterInServer = messages.back().counter;

    const QString channel = isChannel ? messages.front().channelName : "";
    bool deffer = false;
    for (const NewMessageResponse &m: messages) {
        if (!isChannel) {
            CHECK(!m.isChannel, "Message is channel");
        } else {
            CHECK(m.isChannel, "Message not channel");
            CHECK(m.channelName == channel, "Mixed channels messagae");
        }

        const bool isInput = isChannel ? (m.collocutor == address) : m.isInput;
        const QString hashMessage = createHashMessage(m.data); // TODO брать хэш еще и по timestamp
        if (isInput) {
            LOG << "Add message " << address << " " << channel << " " << m.collocutor << " " << m.counter;
            // + channel
            db.addMessage(address, m.collocutor, m.data, m.timestamp, m.counter, isInput, true, true, hashMessage, m.fee);
            const QString collocutorOrChannel = isChannel ? channel : m.collocutor;
            // + isChannel
            const Message::Counter savedPos = db.getLastReadCounterForUserContact(address, collocutorOrChannel); // TODO вместо метода get сделать метод is
            if (savedPos == -1) {
                // + isChannel
                db.setLastReadCounterForUserContact(address, collocutorOrChannel, -1); // Это нужно, чтобы в базе данных отпечаталась связь между отправителем и получателем
            }
        } else {
            // + channel
            const auto idPair = db.findFirstNotConfirmedMessageWithHash(address, hashMessage);
            const auto idDb = idPair.first;
            const Message::Counter counter = idPair.second;
            if (idDb != -1) {
                LOG << "Update message " << address << " " << channel << " " << m.counter;
                // + channel
                db.updateMessage(idDb, m.counter, true);
                // + channel
                if (counter != m.counter && !db.hasMessageWithCounter(address, counter)) {
                    if (!isChannel) {
                        getMessagesFromAddressFromWss(address, counter, counter);
                    } else {
                        getMessagesFromChannelFromWss(address, channel, counter, counter);
                    }
                    deffer = true;
                }
            } else {
                // + channel
                const auto idPair2 = db.findFirstMessageWithHash(address, hashMessage);
                if (idPair2.first == -1) {
                    LOG << "Insert new output message " << address << " " << channel << " " << m.counter;
                    // + channel
                    db.addMessage(address, m.collocutor, m.data, m.timestamp, m.counter, isInput, false, true, hashMessage, m.fee);
                }
            }
        }
    }

    const auto deferrPair = std::make_pair(address, channel);
    if (deffer) {
        LOG << "Deffer message0 " << address << " " << channel;
        deferredMessages[deferrPair].setDeferred(2s);
    } else if (minCounterInServer > currConfirmedCounter + 1) {
        LOG << "Deffer message " << address << " " << channel << " " << minCounterInServer << " " << currConfirmedCounter << " " << maxCounterInServer;
        deferredMessages[deferrPair].setDeferred(2s);
        if (!isChannel) {
            getMessagesFromAddressFromWss(address, currConfirmedCounter + 1, minCounterInServer);
        } else {
            getMessagesFromChannelFromWss(address, channel, currConfirmedCounter + 1, minCounterInServer);
        }
    } else {
        if (!deferredMessages[deferrPair].isDeferred()) {
            if (!isChannel) {
                emit javascriptWrapper.newMessegesSig(address, maxCounterInServer);
            } else {
                // emit channel
            }
        } else {
            LOG << "Deffer message2 " << address << " " << channel << " " << minCounterInServer << " " << currConfirmedCounter << " " << maxCounterInServer;
        }
    }
}

void Messenger::onWssMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument messageJson = QJsonDocument::fromJson(message.toUtf8());
    const ResponseType responseType = getMethodAndAddressResponse(messageJson);

    if (responseType.isError) {
        LOG << "Messenger response error " << responseType.id << " " << responseType.method << " " << responseType.address << " " << responseType.error;
        if (responseType.id != size_t(-1)) {
            invokeCallback(responseType.id, TypedException(TypeErrors::MESSENGER_SERVER_ERROR, responseType.error.toStdString())); // TODO Разбирать ответ от сервера
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
        processMessages(responseType.address, {messages}, messages.isChannel);
    } else if (responseType.method == METHOD::NEW_MSGS) {
        const std::vector<NewMessageResponse> messages = parseNewMessagesResponse(messageJson);
        LOG << "New msgs " << responseType.address << " " << messages.size();
        processMessages(responseType.address, messages, false);
    } else if (responseType.method == METHOD::GET_CHANNEL) {
        const std::vector<NewMessageResponse> messages = parseGetChannelResponse(messageJson);
        LOG << "New msgs " << responseType.address << " " << messages.size();
        processMessages(responseType.address, messages, true);
    } else if (responseType.method == METHOD::SEND_TO_ADDR) {
        LOG << "Send to addr ok " << responseType.address;
        invokeCallback(responseType.id, TypedException());
    } else if (responseType.method == METHOD::GET_MY_CHANNELS) {
        LOG << "Get my channels " << responseType.address;
        const std::vector<ChannelInfo> channelsInfos = parseGetMyChannelsResponse(messageJson);
        processMyChannels(responseType.address, channelsInfos);
    } else if (responseType.method == METHOD::CHANNEL_CREATE) {
        LOG << "Channel create ok " << responseType.address;
        invokeCallback(responseType.id, TypedException());
    } else if (responseType.method == METHOD::CHANNEL_ADD_WRITER) {
        LOG << "Channel add writer ok " << responseType.address;
        invokeCallback(responseType.id, TypedException());
    } else if (responseType.method == METHOD::CHANNEL_DEL_WRITER) {
        LOG << "Channel del writer ok " << responseType.address;
        invokeCallback(responseType.id, TypedException());
    } else if (responseType.method == METHOD::SEND_TO_CHANNEL) {
        LOG << "Send to channel ok " << responseType.address;
        invokeCallback(responseType.id, TypedException());
    } else if (responseType.method == METHOD::ADD_TO_CHANNEL) {
        const ChannelInfo channelInfo = parseAddToChannelResponse(messageJson);
        LOG << "Added to channel ok " << responseType.address << " " << channelInfo.titleSha;
        processAddOrDeleteInChannel(responseType.address, channelInfo, true);
    } else if (responseType.method == METHOD::DEL_FROM_CHANNEL) {
        const ChannelInfo channelInfo = parseDelToChannelResponse(messageJson);
        LOG << "Del to channel ok " << responseType.address << " " << channelInfo.titleSha;
        processAddOrDeleteInChannel(responseType.address, channelInfo, false);
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
        CHECK_TYPED(!pubkey.isEmpty(), TypeErrors::INCOMPLETE_USER_INFO, "Collocutor pubkey not found " + address.toStdString());
        LOG << "Publickey found " << address << " " << pubkey;
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, pubkey, exception));
END_SLOT_WRAPPER
}

void Messenger::onSendMessage(const QString &thisAddress, const QString &toAddress, bool isChannel, QString channel, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    if (!isChannel) {
        channel = "";
    }
    const QString hashMessage = createHashMessage(dataHex);
    // + channel
    Message::Counter lastCnt = db.getMessageMaxCounter(thisAddress);
    if (lastCnt < 0) {
        lastCnt = -1;
    }
    // + channel
    db.addMessage(thisAddress, toAddress, encryptedDataHex, timestamp, lastCnt + 1, false, true, false, hashMessage, fee);
    const size_t idRequest = id.get();
    QString message;
    if (!isChannel) {
        message = makeSendMessageRequest(toAddress, dataHex, pubkeyHex, signHex, fee, timestamp, idRequest);
    } else {
        message = makeSendToChannelRequest(channel, dataHex, fee, timestamp, pubkeyHex, signHex, idRequest);
    }
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
