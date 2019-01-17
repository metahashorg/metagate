#include "Messenger.h"

#include "check.h"
#include "SlotWrapper.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "Paths.h"
#include "utils.h"

#include "MessengerMessages.h"
#include "MessengerJavascript.h"

#include <functional>
using namespace std::placeholders;

#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include <QSettings>

#include <QCryptographicHash>

#include "MessengerDBStorage.h"

namespace messenger {

static QString createHashMessage(const QString &message) {
    return QString(QCryptographicHash::hash(message.toUtf8(), QCryptographicHash::Sha512).toHex());
}

void Messenger::checkChannelTitle(const QString &title) {
    bool isUnicode = false;
    for (int i = 0; i < title.size(); ++i) {
        if (title.at(i).unicode() > 127) {
            isUnicode = true;
        }
    }
    CHECK_TYPED(!isUnicode, TypeErrors::CHANNEL_TITLE_INCORRECT, "Only ascii");
    bool isAlphanumeric = true;
    for (int i = 0; i < title.size(); i++) {
        const auto c = title.at(i);
        if (('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z') || c == '-' || c == '_') {
            // ok
        } else {
            isAlphanumeric = false;
        }
    }
    CHECK_TYPED(isAlphanumeric, TypeErrors::CHANNEL_TITLE_INCORRECT, "Only alphanumerics");
}

QString Messenger::getChannelSha(const QString &title) {
    checkChannelTitle(title);
    return QString(QCryptographicHash::hash(title.toUtf8(), QCryptographicHash::Sha256).toHex());
}

std::vector<QString> Messenger::stringsForSign() {
    return {makeTextForGetMyMessagesRequest(), makeTextForGetChannelRequest(), makeTextForGetChannelsRequest(), makeTextForMsgAppendKeyOnlineRequest(), makeTextForGetMyChannelsRequest()};
}

QString Messenger::makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee) {
    return messenger::makeTextForSignRegisterRequest(address, rsaPubkeyHex, fee);
}

QString Messenger::makeTextForGetPubkeyRequest(const QString &address) {
    return messenger::makeTextForGetPubkeyRequest(address);
}

QString Messenger::makeTextForSendMessageRequest(const QString &address, const QString &dataHex, uint64_t fee, uint64_t timestamp) {
    return messenger::makeTextForSendMessageRequest(address, dataHex, fee, timestamp);
}

QString Messenger::makeTextForChannelCreateRequest(const QString &title, const QString titleSha, uint64_t fee) {
    return messenger::makeTextForChannelCreateRequest(title, titleSha, fee);
}

QString Messenger::makeTextForChannelAddWriterRequest(const QString &titleSha, const QString &address) {
    return messenger::makeTextForChannelAddWriterRequest(titleSha, address);
}

QString Messenger::makeTextForChannelDelWriterRequest(const QString &titleSha, const QString &address) {
    return messenger::makeTextForChannelDelWriterRequest(titleSha, address);
}

QString Messenger::makeTextForSendToChannelRequest(const QString &titleSha, const QString &text, uint64_t fee, uint64_t timestamp) {
    return messenger::makeTextForSendToChannelRequest(titleSha, text, fee, timestamp);
}

static QString getWssServer() {
    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("web_socket/messenger"), "web_socket/messenger setting not found");
    return settings.value("web_socket/messenger").toString();
};

Messenger::Messenger(MessengerJavascript &javascriptWrapper, MessengerDBStorage &db, CryptographicManager &cryptManager, QObject *parent)
    : TimerClass(1s, parent)
    , db(db)
    , javascriptWrapper(javascriptWrapper)
    , cryptManager(cryptManager)
    , wssClient(getWssServer())
{
    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("messenger/saveDecryptedMessage"), "timeout not found");
    isDecryptDataSave = settings.value("messenger/saveDecryptedMessage").toBool();

    CHECK(connect(this, &Messenger::callbackCall, this, &Messenger::onCallbackCall), "not connect onCallbackCall");

    CHECK(connect(this, SIGNAL(timerEvent()), this, SLOT(onTimerEvent())), "not connect onTimerEvent");
    CHECK(connect(&wssClient, &WebSocketClient::messageReceived, this, &Messenger::onWssMessageReceived), "not connect wssClient");
    CHECK(connect(this, SIGNAL(startedEvent()), this, SLOT(onRun())), "not connect run");

    CHECK(connect(this, &Messenger::registerAddress, this, &Messenger::onRegisterAddress), "not connect onRegisterAddress");
    CHECK(connect(this, &Messenger::savePubkeyAddress, this, &Messenger::onSavePubkeyAddress), "not connect onGetPubkeyAddress");
    CHECK(connect(this, &Messenger::getPubkeyAddress, this, &Messenger::onGetPubkeyAddress), "not connect onGetPubkeyAddress");
    CHECK(connect(this, &Messenger::sendMessage, this, &Messenger::onSendMessage), "not connect onSendMessage");
    CHECK(connect(this, &Messenger::signedStrings, this, &Messenger::onSignedStrings), "not connect onSignedStrings");
    CHECK(connect(this, &Messenger::getLastMessage, this, &Messenger::onGetLastMessage), "not connect onGetLastMessage");
    CHECK(connect(this, &Messenger::getSavedPos, this, &Messenger::onGetSavedPos), "not connect onGetSavedPos");
    CHECK(connect(this, &Messenger::getSavedsPos, this, &Messenger::onGetSavedsPos), "not connect onGetSavedsPos");
    CHECK(connect(this, &Messenger::savePos, this, &Messenger::onSavePos), "not connect onGetSavedPos");
    CHECK(connect(this, &Messenger::getCountMessages, this, &Messenger::onGetCountMessages), "not connect onGetCountMessages");
    CHECK(connect(this, &Messenger::getHistoryAddress, this, &Messenger::onGetHistoryAddress), "not connect onGetHistoryAddress");
    CHECK(connect(this, &Messenger::getHistoryAddressAddress, this, &Messenger::onGetHistoryAddressAddress), "not connect onGetHistoryAddressAddress");
    CHECK(connect(this, &Messenger::getHistoryAddressAddressCount, this, &Messenger::onGetHistoryAddressAddressCount), "not connect onGetHistoryAddressAddressCount");
    CHECK(connect(this, &Messenger::createChannel, this, &Messenger::onCreateChannel), "not connect onCreateChannel");
    CHECK(connect(this, &Messenger::addWriterToChannel, this, &Messenger::onAddWriterToChannel), "not connect onAddWriterToChannel");
    CHECK(connect(this, &Messenger::delWriterFromChannel, this, &Messenger::onDelWriterFromChannel), "not connect onDelWriterFromChannel");
    CHECK(connect(this, &Messenger::getChannelList, this, &Messenger::onGetChannelList), "not connect onGetChannelList");

    qRegisterMetaType<uint64_t>("uint64_t");
    qRegisterMetaType<Callback>("Callback");
    qRegisterMetaType<Message::Counter>("Message::Counter");
    qRegisterMetaType<GetMessagesCallback>("GetMessagesCallback");
    qRegisterMetaType<SavePosCallback>("SavePosCallback");
    qRegisterMetaType<GetSavedPosCallback>("GetSavedPosCallback");
    qRegisterMetaType<GetSavedsPosCallback>("GetSavedsPosCallback");
    qRegisterMetaType<Messenger::RegisterAddressCallback>("Messenger::RegisterAddressCallback");
    qRegisterMetaType<SignedStringsCallback>("SignedStringsCallback");
    qRegisterMetaType<SavePubkeyCallback>("SavePubkeyCallback");
    qRegisterMetaType<GetPubkeyAddressCallback>("GetPubkeyAddressCallback");
    qRegisterMetaType<SendMessageCallback>("SendMessageCallback");
    qRegisterMetaType<GetCountMessagesCallback>("GetCountMessagesCallback");
    qRegisterMetaType<CreateChannelCallback>("CreateChannelCallback");
    qRegisterMetaType<AddWriterToChannelCallback>("AddWriterToChannelCallback");
    qRegisterMetaType<DelWriterToChannelCallback>("DelWriterToChannelCallback");
    qRegisterMetaType<GetChannelListCallback>("GetChannelListCallback");

    qRegisterMetaType<std::vector<QString>>("std::vector<QString>");

    moveToThread(&thread1);

    wssClient.start();
}

void Messenger::onCallbackCall(const std::function<void()> &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void Messenger::invokeCallback(size_t requestId, const TypedException &exception) {
    CHECK(requestId != size_t(-1), "Incorrect request id");
    const auto found = callbacks.find(requestId);
    CHECK(found != callbacks.end(), "Not found callback for request " + std::to_string(requestId));
    const auto &callbackPair = found->second;
    const ResponseCallbacks callback = callbackPair.first; // копируем
    const bool isNativeCallback = callbackPair.second;
    callbacks.erase(found);
    if (isNativeCallback) {
        callback(exception);
    } else {
        emit javascriptWrapper.callbackCall(std::bind(callback, exception));
    }
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
    emit wssClient.setHelloString(std::vector<QString>{}, "Messenger");
}

void Messenger::addAddressToMonitored(const QString &address) {
    LOG << "Add address to monitored " << address;
    const QString pubkeyHex = db.getUserPublicKey(address);
    CHECK_TYPED(!pubkeyHex.isEmpty(), TypeErrors::INCOMPLETE_USER_INFO, "user pubkey not found " + address.toStdString());

    const QString signHexChannels = getSignFromMethod(address, makeTextForGetMyChannelsRequest());
    const QString messageGetMyChannels = makeGetMyChannelsRequest(pubkeyHex, signHexChannels, id.get());
    emit wssClient.addHelloString(messageGetMyChannels, "Messenger");
    emit wssClient.sendMessage(messageGetMyChannels);

    const QString signHex = getSignFromMethod(address, makeTextForMsgAppendKeyOnlineRequest());
    const QString message = makeAppendKeyOnlineRequest(pubkeyHex, signHex, id.get());
    emit wssClient.addHelloString(message, "Messenger");
    emit wssClient.sendMessage(message);
}

void Messenger::processMyChannels(const QString &address, const std::vector<ChannelInfo> &channels) {
    db.setChannelsNotVisited(address);
    const DBStorage::DbId userId = db.getUserId(address);
    for (const ChannelInfo &channel: channels) {
        const DBStorage::DbId dbId = db.getChannelForUserShaName(address, channel.titleSha);
        if (dbId != -1) {
            db.updateChannel(dbId, true);
        } else {
            db.addChannel(userId, channel.title, channel.titleSha, channel.admin == address, channel.admin, false, true, true);
            db.setLastReadCounterForUserContact(address, channel.titleSha, -1, true);
        }
    }
    db.setWriterForNotVisited(address);
    for (const ChannelInfo &channel: channels) {
        const Message::Counter counter = db.getMessageMaxCounter(address, channel.titleSha);
        if (counter < channel.counter) {
            getMessagesFromChannelFromWss(address, channel.titleSha, counter + 1, channel.counter);
        }
    }
}

void Messenger::processAddOrDeleteInChannel(const QString &address, const ChannelInfo &channel, bool isAdd) {
    if (!isAdd) {
        db.setChannelIsWriterForUserShaName(address, channel.titleSha, false);
        emit javascriptWrapper.deletedFromChannelSig(address, channel.title, channel.titleSha, channel.admin);
    } else {
        const DBStorage::DbId userId = db.getUserId(address);
        db.addChannel(userId, channel.title, channel.titleSha, channel.admin == address, channel.admin, false, true, false);
        const Message::Counter cnt = channel.counter;
        if (cnt != -1) {
            getMessagesFromChannelFromWss(address, channel.titleSha, 0, cnt);
        }
        emit javascriptWrapper.addedToChannelSig(address, channel.title, channel.titleSha, channel.admin, channel.counter);
    }
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
            const Message::Counter lastCnt = db.getMessageMaxCounter(address, "");
            LOG << "Defferred process message " << address << " " << channel << " " << lastCnt;
            if (channel.isEmpty()) {
                emit javascriptWrapper.newMessegesSig(address, lastCnt);
            } else {
                emit javascriptWrapper.newMessegesChannelSig(address, channel, lastCnt);
            }
        }
    }
END_SLOT_WRAPPER
}

void Messenger::processMessages(const QString &address, const std::vector<NewMessageResponse> &messages, bool isChannel) {
    CHECK(!messages.empty(), "Empty messages");

    std::vector<Message> msgs;
    msgs.reserve(messages.size());
    std::transform(messages.begin(), messages.end(), std::back_inserter(msgs), [address, isChannel](const NewMessageResponse &m) {
        Message message;
        message.channel = m.channelName;
        message.isChannel = m.isChannel;
        message.collocutor = m.collocutor;
        message.counter = m.counter;
        message.data = m.data;
        message.isDecrypted = false;
        message.fee = m.fee;
        const QString hashMessage = createHashMessage(m.data); // TODO брать хэш еще и по timestamp
        message.hash = hashMessage;
        message.isConfirmed = true;
        const bool isInput = isChannel ? (m.collocutor == address) : m.isInput;
        message.isCanDecrypted = isInput;
        message.isInput = isInput;
        message.timestamp = m.timestamp;
        message.username = address;

        return message;
    });

    const auto nextProcess = [this, isChannel, address](const std::vector<Message> &messages) {
        CHECK(!messages.empty(), "Empty messages");
        const QString channel = isChannel ? messages.front().channel : "";

        const Message::Counter currConfirmedCounter = db.getMessageMaxConfirmedCounter(address);
        CHECK(std::is_sorted(messages.begin(), messages.end()), "Messages not sorted");
        const Message::Counter minCounterInServer = messages.front().counter;
        const Message::Counter maxCounterInServer = messages.back().counter;

        bool deffer = false;
        for (const Message &m: messages) {
            if (!isChannel) {
                CHECK(!m.isChannel, "Message is channel");
            } else {
                CHECK(m.isChannel, "Message not channel");
                CHECK(m.channel == channel, "Mixed channels messagae");
            }
            CHECK(address == m.username, "Incorrect message");

            if (m.isInput) {
                LOG << "Add message " << m.username << " " << channel << " " << m.collocutor << " " << m.counter;
                db.addMessage(m);
                const QString collocutorOrChannel = isChannel ? channel : m.collocutor;
                const Message::Counter savedPos = db.getLastReadCounterForUserContact(m.username, collocutorOrChannel, isChannel); // TODO вместо метода get сделать метод is
                if (savedPos == -1) {
                    db.setLastReadCounterForUserContact(m.username, collocutorOrChannel, -1, isChannel); // Это нужно, чтобы в базе данных отпечаталась связь между отправителем и получателем
                }
            } else {
                const auto idPair = db.findFirstNotConfirmedMessageWithHash(m.username, m.hash, channel);
                const auto idDb = idPair.first;
                const Message::Counter counter = idPair.second;
                if (idDb != -1) {
                    LOG << "Update message " << m.username << " " << channel << " " << m.counter;
                    db.updateMessage(idDb, m.counter, true);
                    if (counter != m.counter && !db.hasMessageWithCounter(m.username, counter, channel)) {
                        if (!isChannel) {
                            getMessagesFromAddressFromWss(m.username, counter, counter);
                        } else {
                            getMessagesFromChannelFromWss(m.username, channel, counter, counter);
                        }
                        deffer = true;
                    }
                } else {
                    const auto idPair2 = db.findFirstMessageWithHash(m.username, m.hash, channel);
                    if (idPair2.first == -1) {
                        LOG << "Insert new output message " << m.username << " " << channel << " " << m.counter;
                        db.addMessage(m);
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
                    emit javascriptWrapper.newMessegesChannelSig(address, channel, maxCounterInServer);
                }
            } else {
                LOG << "Deffer message2 " << address << " " << channel << " " << minCounterInServer << " " << currConfirmedCounter << " " << maxCounterInServer;
            }
        }
    };

    if (!isDecryptDataSave) {
        nextProcess(msgs);
    } else {
        emit cryptManager.tryDecryptMessages(msgs, address, CryptographicManager::DecryptMessagesCallback(nextProcess, [](const TypedException &exception) {
            LOG << "Error " << exception.numError << " " << exception.description;
        }, std::bind(&Messenger::callbackCall, this, _1), false));
    }
}

void Messenger::onWssMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    const QJsonDocument messageJson = QJsonDocument::fromJson(message.toUtf8());
    const ResponseType responseType = getMethodAndAddressResponse(messageJson);

    if (responseType.isError) {
        LOG << "Messenger response error " << responseType.id << " " << responseType.method << " " << responseType.address << " " << responseType.error;
        if (responseType.id != size_t(-1)) {
            TypedException exception;
            if (responseType.errorType == ResponseType::ERROR_TYPE::ADDRESS_EXIST) {
                exception = TypedException(TypeErrors::MESSENGER_SERVER_ERROR_ADDRESS_EXIST, responseType.error.toStdString());
            } else if (responseType.errorType == ResponseType::ERROR_TYPE::SIGN_OR_ADDRESS_INVALID) {
                exception = TypedException(TypeErrors::MESSENGER_SERVER_ERROR_SIGN_OR_ADDRESS_INVALID, responseType.error.toStdString());
            } else if (responseType.errorType == ResponseType::ERROR_TYPE::INCORRECT_JSON) {
                exception = TypedException(TypeErrors::MESSENGER_SERVER_ERROR_INCORRECT_JSON, responseType.error.toStdString());
            } else if (responseType.errorType == ResponseType::ERROR_TYPE::ADDRESS_NOT_FOUND) {
                exception = TypedException(TypeErrors::MESSENGER_SERVER_ERROR_ADDRESS_NOT_FOUND, responseType.error.toStdString());
            } else if (responseType.errorType == ResponseType::ERROR_TYPE::CHANNEL_EXIST) {
                exception = TypedException(TypeErrors::MESSENGER_SERVER_ERROR_CHANNEL_EXIST, responseType.error.toStdString());
            } else if (responseType.errorType == ResponseType::ERROR_TYPE::CHANNEL_NOT_PERMISSION) {
                exception = TypedException(TypeErrors::MESSENGER_SERVER_ERROR_CHANNEL_NOT_PERMISSION, responseType.error.toStdString());
            } else if (responseType.errorType == ResponseType::ERROR_TYPE::CHANNEL_NOT_FOUND) {
                exception = TypedException(TypeErrors::MESSENGER_SERVER_ERROR_CHANNEL_NOT_FOUND, responseType.error.toStdString());
            } else {
                exception = TypedException(TypeErrors::MESSENGER_SERVER_ERROR_OTHER, responseType.error.toStdString());
            }

            invokeCallback(responseType.id, exception); // TODO Разбирать ответ от сервера
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
        callback.emitFunc(TypedException(), isNew);
        return;
    }
    const size_t idRequest = id.get();
    const QString message = makeRegisterRequest(rsaPubkeyHex, pubkeyAddressHex, signHex, fee, idRequest);
    const auto callbackWrap = [this, callback, isNew, address, pubkeyAddressHex, isForcibly](const TypedException &exception) {
        if (!exception.isSet() || isForcibly) { // TODO убрать isForcibly
            LOG << "Set user pubkey " << address << " " << pubkeyAddressHex;
            db.setUserPublicKey(address, pubkeyAddressHex);
        }
        callback.emitFunc(exception, isNew);
    };
    callbacks[idRequest] = std::make_pair(callbackWrap, true);
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
        callback.emitFunc(TypedException(), isNew);
        return;
    }
    const size_t idRequest = id.get();
    const QString message = makeGetPubkeyRequest(address, pubkeyHex, signHex, idRequest);
    callbacks[idRequest] = std::make_pair(std::bind(callback, _1, isNew), false);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onGetPubkeyAddress(const QString &address, const GetPubkeyAddressCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString pubkey = "";
    const TypedException exception = apiVrapper2([&, this] {
        pubkey = db.getContactPublicKey(address);
        CHECK_TYPED(!pubkey.isEmpty(), TypeErrors::INCOMPLETE_USER_INFO, "Collocutor pubkey not found " + address.toStdString());
        LOG << "Publickey found " << address << " " << pubkey;
    });
    callback.emitFunc(exception, pubkey);
END_SLOT_WRAPPER
}

void Messenger::onSendMessage(const QString &thisAddress, const QString &toAddress, bool isChannel, QString channel, const QString &dataHex, const QString &decryptedDataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    if (!isChannel) {
        channel = "";
    }
    const QString hashMessage = createHashMessage(dataHex);
    Message::Counter lastCnt = db.getMessageMaxCounter(thisAddress, channel);
    if (lastCnt < 0) {
        lastCnt = -1;
    }
    QString dData;
    if (isDecryptDataSave) {
        dData = QString::fromStdString(fromHex(decryptedDataHex.toStdString()));
    } else {
        dData = "";
    }
    db.addMessage(thisAddress, toAddress, encryptedDataHex, dData, isDecryptDataSave, timestamp, lastCnt + 1, false, true, false, hashMessage, fee, channel);
    const size_t idRequest = id.get();
    QString message;
    if (!isChannel) {
        message = makeSendMessageRequest(toAddress, dataHex, pubkeyHex, signHex, fee, timestamp, idRequest);
    } else {
        message = makeSendToChannelRequest(channel, dataHex, fee, timestamp, pubkeyHex, signHex, idRequest);
    }
    callbacks[idRequest] = std::make_pair(callback, false);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onGetSavedPos(const QString &address, bool isChannel, const QString &collocutorOrChannel, const GetSavedPosCallback &callback) {
BEGIN_SLOT_WRAPPER
    Message::Counter lastCounter;
    const TypedException exception = apiVrapper2([&, this] {
        lastCounter = db.getLastReadCounterForUserContact(address, collocutorOrChannel, isChannel);
    });
    callback.emitFunc(exception, lastCounter);
END_SLOT_WRAPPER
}

void Messenger::onGetSavedsPos(const QString &address, bool isChannel, const GetSavedsPosCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<MessengerDBStorage::NameCounterPair> pos;
    const TypedException exception = apiVrapper2([&, this] {
        if (isChannel) {
            pos = db.getLastReadCountersForChannels(address);
        } else {
            pos = db.getLastReadCountersForContacts(address);
        }
    });
    callback.emitFunc(exception, pos);
END_SLOT_WRAPPER
}

void Messenger::onSavePos(const QString &address, bool isChannel, const QString &collocutorOrChannel, Message::Counter pos, const SavePosCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        db.setLastReadCounterForUserContact(address, collocutorOrChannel, pos, isChannel);
    });
    emit javascriptWrapper.callbackCall(std::bind(callback, exception));
END_SLOT_WRAPPER
}

void Messenger::onGetLastMessage(const QString &address, bool isChannel, QString channel, const GetSavedPosCallback &callback) {
BEGIN_SLOT_WRAPPER
    Message::Counter lastCounter;
    const TypedException exception = apiVrapper2([&, this] {
        if (!isChannel) {
            channel = "";
        }
        lastCounter = db.getMessageMaxCounter(address, channel);
    });
    callback.emitFunc(exception, lastCounter);
END_SLOT_WRAPPER
}

void Messenger::onGetCountMessages(const QString &address, const QString &collocutor, Message::Counter from, const GetCountMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    Message::Counter lastCounter = 0;
    const TypedException exception = apiVrapper2([&, this] {
        lastCounter = db.getMessagesCountForUserAndDest(address, collocutor, from);
    });
    callback.emitFunc(exception, lastCounter);
END_SLOT_WRAPPER
}

void Messenger::onGetHistoryAddress(QString address, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Message> messages;
    const TypedException exception = apiVrapper2([&, this] {
        messages = db.getMessagesForUser(address, from, to);
    });
    callback.emitFunc(exception, messages);
END_SLOT_WRAPPER
}

void Messenger::onGetHistoryAddressAddress(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Message> messages;
    const TypedException exception = apiVrapper2([&, this] {
        messages = db.getMessagesForUserAndDest(address, collocutorOrChannel, from, to, isChannel);
    });
    callback.emitFunc(exception, messages);
END_SLOT_WRAPPER
}

void Messenger::onGetHistoryAddressAddressCount(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter count, Message::Counter to, const GetMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Message> messages;
    const TypedException exception = apiVrapper2([&, this] {
        messages = db.getMessagesForUserAndDestNum(address, collocutorOrChannel, to, count, isChannel);
    });
    callback.emitFunc(exception, messages);
END_SLOT_WRAPPER
}

void Messenger::onCreateChannel(const QString &address, const QString &title, const QString &titleSha, const QString &pubkeyHex, const QString &signHex, uint64_t fee, const CreateChannelCallback &callback) {
BEGIN_SLOT_WRAPPER
    const size_t idRequest = id.get();
    const QString message = makeCreateChannelRequest(title, titleSha, fee, pubkeyHex, signHex, idRequest);
    const auto callbackWrap = [this, callback, address, title, titleSha](const TypedException &exception) {
        if (!exception.isSet()) {
            const DBStorage::DbId userId = db.getUserId(address);
            db.addChannel(userId, title, titleSha, true, address, false, true, true);
            db.setLastReadCounterForUserContact(address, titleSha, -1, true);
        }
        emit javascriptWrapper.callbackCall(std::bind(callback, exception));
    };
    callbacks[idRequest] = std::make_pair(callbackWrap, true);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onAddWriterToChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const AddWriterToChannelCallback &callback) {
BEGIN_SLOT_WRAPPER
    const size_t idRequest = id.get();
    const QString message = makeChannelAddWriterRequest(titleSha, address, pubkeyHex, signHex, idRequest);
    callbacks[idRequest] = std::make_pair(std::bind(callback, _1), false);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onDelWriterFromChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const DelWriterToChannelCallback &callback) {
BEGIN_SLOT_WRAPPER
    const size_t idRequest = id.get();
    const QString message = makeChannelDelWriterRequest(titleSha, address, pubkeyHex, signHex, idRequest);
    callbacks[idRequest] = std::make_pair(std::bind(callback, _1), false);
    emit wssClient.sendMessage(message);
END_SLOT_WRAPPER
}

void Messenger::onGetChannelList(const QString &address, const GetChannelListCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<ChannelInfo> channels;
    const TypedException exception = apiVrapper2([&, this] {
        channels = db.getChannelsWithLastReadCounters(address);
    });
    callback.emitFunc(exception, channels);
END_SLOT_WRAPPER
}

}
