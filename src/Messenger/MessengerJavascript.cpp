#include "MessengerJavascript.h"

#include "check.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "SlotWrapper.h"
#include "utils.h"
#include "Paths.h"
#include "QRegister.h"

#include "CryptographicManager.h"
#include "Messenger.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "Wallet.h"

#include "auth/Auth.h"
#include "JavascriptWrapper.h"

using namespace std::placeholders;

namespace messenger {

MessengerJavascript::MessengerJavascript(auth::Auth &authManager, CryptographicManager &cryptoManager, QObject *parent)
    : QObject(parent)
    , authManager(authManager)
    , cryptoManager(cryptoManager)
{
    CHECK(connect(this, &MessengerJavascript::callbackCall, this, &MessengerJavascript::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &MessengerJavascript::newMessegesSig, this, &MessengerJavascript::onNewMesseges), "not connect onNewMesseges");
    CHECK(connect(this, &MessengerJavascript::addedToChannelSig, this, &MessengerJavascript::onAddedToChannel), "not connect onAddedToChannel");
    CHECK(connect(this, &MessengerJavascript::deletedFromChannelSig, this, &MessengerJavascript::onDeletedFromChannel), "not connect onDeletedFromChannel");
    CHECK(connect(this, &MessengerJavascript::newMessegesChannelSig, this, &MessengerJavascript::onNewMessegesChannel), "not connect onNewMessegesChannel");
    CHECK(connect(this, &MessengerJavascript::requiresPubkeySig, this, &MessengerJavascript::onRequiresPubkey), "not connect onRequiresPubkey");
    CHECK(connect(this, &MessengerJavascript::collocutorAddedPubkeySig, this, &MessengerJavascript::onCollocutorAddedPubkey), "not connect onCollocutorAddedPubkey");

    CHECK(connect(&authManager, &auth::Auth::logined, this, &MessengerJavascript::onLogined), "not connect onLogined");

    Q_REG(MessengerJavascript::Callback, "MessengerJavascript::Callback");

    defaultWalletPath = getWalletPath();
    defaultUserName = JavascriptWrapper::defaultUsername;

    signalFunc = std::bind(&MessengerJavascript::callbackCall, this, _1);

    emit authManager.reEmit();
}

void MessengerJavascript::setMessenger(Messenger &m) {
    messenger = &m;
    emit authManager.reEmit();
}

void MessengerJavascript::onCallbackCall(const std::function<void()> &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

template<typename... Args>
void MessengerJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

static QJsonDocument messagesToJson(const std::vector<Message> &messages) {
    QJsonArray messagesArrJson;
    for (const Message &message: messages) {
        QJsonObject messageJson;

        messageJson.insert("collocutor", message.collocutor);
        messageJson.insert("isInput", message.isInput);
        messageJson.insert("timestamp", QString::fromStdString(std::to_string(message.timestamp)));
        messageJson.insert("data", message.decryptedDataHex);
        messageJson.insert("isDecrypter", message.isDecrypted);
        messageJson.insert("counter", QString::fromStdString(std::to_string(message.counter)));
        messageJson.insert("fee", QString::fromStdString(std::to_string(message.fee)));
        messageJson.insert("isConfirmed", message.isConfirmed);
        if (message.isChannel) {
            messageJson.insert("channel", message.channel);
        }

        messagesArrJson.push_back(messageJson);
    }
    return QJsonDocument(messagesArrJson);
}

static QJsonDocument channelListToJson(const std::vector<ChannelInfo> &channels) {
    QJsonArray messagesArrJson;
    for (const ChannelInfo &channel: channels) {
        QJsonObject messageJson;

        messageJson.insert("title", channel.title);
        messageJson.insert("titleSha", channel.titleSha);
        messageJson.insert("admin", channel.admin);
        messageJson.insert("isWriter", channel.isWriter);
        messageJson.insert("saved_pos", QString::fromStdString(std::to_string(channel.counter)));
        messageJson.insert("fee", QString::fromStdString(std::to_string(channel.fee)));
        messagesArrJson.push_back(messageJson);
    }
    return QJsonDocument(messagesArrJson);
}

static QJsonDocument allPosToJson(const std::vector<std::pair<QString, Message::Counter>> &pos) {
    QJsonArray messagesArrJson;
    for (const auto &pair: pos) {
        QJsonObject messageJson;
        messageJson.insert("address", pair.first);
        messageJson.insert("counter", pair.second);
        messagesArrJson.push_back(messageJson);
    }
    return QJsonDocument(messagesArrJson);
}

void MessengerJavascript::getHistoryAddress(QString address, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressJs";

    LOG << "get messages " << address << " " << from << " " << to;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, result);
    };

    const auto errorFunc = [address, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddress(address, fromC, toC, Messenger::GetMessagesCallback([this, address, makeFunc, errorFunc](const std::vector<Message> &messages) {
            LOG << "Count messages " << address << " " << messages.size();
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, makeFunc](const std::vector<Message> &messages){
                makeFunc(TypedException(), address, messagesToJson(messages));
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddress(QString address, QString collocutor, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressJs";

    LOG << "get messages " << address << " " << collocutor << " " << from << " " << to;

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, result);
    };

    const auto errorFunc = [address, collocutor, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, collocutor, QJsonDocument());
    };

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddressAddress(address, false, collocutor, fromC, toC, Messenger::GetMessagesCallback([this, address, collocutor, makeFunc, errorFunc](const std::vector<Message> &messages) {
            LOG << "Count messages " << address << " " << collocutor << " " << messages.size();
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, collocutor, makeFunc](const std::vector<Message> &messages){
                makeFunc(TypedException(), address, collocutor, messagesToJson(messages));
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, collocutor, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddressCount(QString address, QString collocutor, QString count, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressCountJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, result);
    };

    const auto errorFunc = [address, collocutor, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, collocutor, QJsonDocument());
    };

    LOG << "get messagesC " << address << " " << collocutor << " " << count << " " << to;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter countC = count.toLongLong(&isValid);
        CHECK(isValid, "count field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddressAddressCount(address, false, collocutor, countC, toC, Messenger::GetMessagesCallback([this, address, collocutor, makeFunc, errorFunc](const std::vector<Message> &messages) {
            LOG << "Count messagesC " << address << " " << collocutor << " " << messages.size();
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, collocutor, makeFunc](const std::vector<Message> &messages){
                makeFunc(TypedException(), address, collocutor, messagesToJson(messages));
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, collocutor, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::registerAddress(bool isForcibly, QString address, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgAddressAppendToMessengerJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    const auto errorFunc = [makeFunc](const TypedException &exception) {
        makeFunc(exception, QString("Not ok"));
    };

    LOG << "registerAddress " << address;

    const TypedException exception = apiVrapper2([&, this](){
        const auto processFunc = [this, address, isForcibly, makeFunc, errorFunc](bool isNew) {
            if (isNew || isForcibly) {
                const std::vector<QString> messagesForSign = Messenger::stringsForSign();
                emit cryptoManager.signMessages(address, messagesForSign, CryptographicManager::SignMessagesCallback([this, address, makeFunc, errorFunc](const QString &pubkey, const std::vector<QString> &sign){
                    emit messenger->signedStrings(address, sign, Messenger::SignedStringsCallback([address, makeFunc](){
                        LOG << "Address registered " << address;
                        makeFunc(TypedException(), QString("Ok"));
                    }, errorFunc, signalFunc));
                }, errorFunc, signalFunc));
            } else {
                makeFunc(TypedException(), QString("Ok"));
            }
        };

        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "Fee field incorrect");
        emit cryptoManager.getPubkeyRsa(address, CryptographicManager::GetPubkeyRsaCallback([this, address, isForcibly, fee, makeFunc, errorFunc, processFunc](const QString &pubkeyRsa){
            const QString messageToSign = Messenger::makeTextForSignRegisterRequest(address, pubkeyRsa, fee);
            emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, isForcibly, fee, makeFunc, errorFunc, processFunc, pubkeyRsa](const QString &pubkey, const QString &sign){
                emit messenger->registerAddress(isForcibly, address, pubkeyRsa, pubkey, sign, fee, Messenger::RegisterAddressCallback(processFunc, [isForcibly, processFunc, errorFunc](const TypedException &exception) {
                    if (isForcibly) {
                        processFunc(true);
                    } else {
                        errorFunc(exception);
                    }
                }, signalFunc));
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, QString("Not ok"));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::savePublicKeyCollocutor(bool isForcibly, QString address, QString collocutor) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgPublicKeyCollocutorGettedJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor);
    };

    const auto errorFunc = [address, collocutor, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, collocutor);
    };

    LOG << "savePublicKeyCollocutor " << address << " " << collocutor;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForGetPubkeyRequest(collocutor);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, collocutor, isForcibly, makeFunc, errorFunc](const QString &pubkey, const QString &sign){
            emit messenger->savePubkeyAddress(isForcibly, collocutor, pubkey, sign, Messenger::SavePubkeyCallback([this, address, collocutor, makeFunc](bool /*isNew*/) {
                LOG << "Pubkey saved " << collocutor;
                makeFunc(TypedException(), address, collocutor);
            }, [this, address, collocutor, makeFunc, errorFunc](const TypedException &exception) {
                if (exception.numError != TypeErrors::MESSENGER_SERVER_ERROR_ADDRESS_NOT_FOUND) {
                    errorFunc(exception);
                    return;
                }
                const QString messageToSign = Messenger::makeTextForWantToTalkRequest(collocutor);
                emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, collocutor, makeFunc, errorFunc](const QString &pubkey, const QString &sign){
                    emit messenger->wantToTalk(collocutor, pubkey, sign, Messenger::WantToTalkCallback([this, address, collocutor, makeFunc](){
                        LOG << "Want to talk " << address << " " << collocutor;
                        makeFunc(TypedException(TypeErrors::MESSENGER_SERVER_ERROR_ADDRESS_NOT_FOUND, "Not found"), address, collocutor);
                    }, errorFunc, signalFunc));
                }, errorFunc, signalFunc));
            }, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        errorFunc(exception);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::sendMessage(QString address, QString collocutor, QString dataHex, QString timestampStr, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgMessageSendedJs";

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor);
    };

    const auto errorFunc = [address, collocutor, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, collocutor);
    };

    LOG << "sendMessage " << " " << address << " " << collocutor << " " << timestampStr << " " << feeStr;

    const TypedException exception = apiVrapper2([&, this](){
        CHECK(!dataHex.isEmpty(), "Empty message");
        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "fee field incorrect");
        const uint64_t timestamp = timestampStr.toULongLong(&isValid);
        CHECK(isValid, "timestamp field incorrect");
        emit messenger->getPubkeyAddress(collocutor, Messenger::GetPubkeyAddressCallback([this, makeFunc, errorFunc, address, collocutor, dataHex, fee, timestamp](const QString &pubkey) mutable {
            emit cryptoManager.encryptDataRsa(dataHex, pubkey, CryptographicManager::EncryptMessageCallback([this, address, collocutor, makeFunc, errorFunc, fee, timestamp, dataHex](const QString &encryptedDataToWss) {
                const QString messageToSign = Messenger::makeTextForSendMessageRequest(collocutor, encryptedDataToWss, fee, timestamp);
                emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, collocutor, makeFunc, errorFunc, fee, timestamp, dataHex, encryptedDataToWss](const QString &pubkey, const QString &sign) {
                    emit cryptoManager.encryptDataPrivateKey(dataHex, address, CryptographicManager::EncryptMessageCallback([this, address, collocutor, makeFunc, errorFunc, fee, timestamp, dataHex, encryptedDataToWss, pubkey, sign](const QString &encryptedDataToBd) {
                        emit messenger->sendMessage(address, collocutor, false, "", encryptedDataToWss, dataHex, pubkey, sign, fee, timestamp, encryptedDataToBd, Messenger::SendMessageCallback([this, makeFunc, address, collocutor]() {
                            LOG << "Message sended " << address << " " << collocutor;
                            makeFunc(TypedException(), address, collocutor);
                        }, errorFunc, signalFunc));
                    }, errorFunc, signalFunc));
                }, errorFunc, signalFunc));
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, collocutor);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getLastMessageNumber(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgLastMessegesJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const Message::Counter &pos) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, pos);
    };

    const auto errorFunc = [address, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, 0);
    };

    LOG << "getLastMessageNumber " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getLastMessage(address, false, "", Messenger::GetSavedPosCallback([this, makeFunc, address](const Message::Counter &pos) {
            LOG << "Last message number " << address << " " << pos;
            makeFunc(TypedException(), address, pos);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, 0);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedPos(QString address, const QString &collocutor) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedPosJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const Message::Counter &pos) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, pos);
    };

    const auto errorFunc = [address, collocutor, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, collocutor, 0);
    };

    LOG << "getSavedPos " << address << " " << collocutor;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedPos(address, false, collocutor, Messenger::GetSavedPosCallback([this, makeFunc, address, collocutor](const Message::Counter &pos) {
            LOG << "Saved pos " << address << " " << collocutor << " " << pos;
            makeFunc(TypedException(), address, collocutor, pos);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, collocutor, 0);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedsPos(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedsPosJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, result);
    };

    const auto errorFunc = [address, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, QJsonDocument());
    };

    LOG << "getSavedsPos " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedsPos(address, false, Messenger::GetSavedsPosCallback([this, makeFunc, address](const std::vector<std::pair<QString, Message::Counter>> &pos) {
            const QJsonDocument result(allPosToJson(pos));

            LOG << "Get saveds pos " << pos.size();
            makeFunc(TypedException(), address, result);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::savePos(QString address, const QString &collocutor, QString counterStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgStorePosJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, result);
    };

    const auto errorFunc = [address, collocutor, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, collocutor, "Not ok");
    };

    LOG << "savePos " << address << " " << collocutor << " " << counterStr;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter counter = counterStr.toLongLong(&isValid);
        CHECK(isValid, "counter field invalid");
        emit messenger->savePos(address, false, collocutor, counter, Messenger::SavePosCallback([this, makeFunc, address, collocutor](){
            LOG << "Save pos ok " << address << " " << collocutor;
            makeFunc(TypedException(), address, collocutor, "Ok");
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, collocutor, "Not ok");
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getCountMessages(QString address, const QString &collocutor, QString from) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgCountMessagesJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const Message::Counter &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, result);
    };

    const auto errorFunc = [address, collocutor, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, collocutor, 0);
    };

    LOG << "getCountMessages " << address << " " << collocutor << " " << from;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter fromI = from.toLongLong(&isValid);
        CHECK(isValid, "from field invalid");
        emit messenger->getCountMessages(address, collocutor, fromI, Messenger::GetCountMessagesCallback([this, makeFunc, address, collocutor, fromI](const Message::Counter &count) {
            LOG << "Count messages " << address << " " << collocutor << " " << fromI << " " << count;
            makeFunc(TypedException(), address, collocutor, count);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, collocutor, 0);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::createChannel(QString address, QString channelTitle, QString fee) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgChannelCreateJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &channel, const QString &channelSha) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channel, channelSha);
    };

    LOG << "channel create " << address << " " << channelTitle << " " << fee;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter feeI = fee.toLongLong(&isValid);
        CHECK(isValid, "fee field invalid");
        const QString titleSha = Messenger::getChannelSha(channelTitle);

        const auto errorFunc = [address, channelTitle, titleSha, makeFunc](const TypedException &exception) {
            makeFunc(exception, address, channelTitle, titleSha);
        };

        const QString messageToSign = Messenger::makeTextForChannelCreateRequest(channelTitle, titleSha, feeI);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, channelTitle, titleSha, feeI, makeFunc, errorFunc](const QString &pubkey, const QString &sign) {
            emit messenger->createChannel(address, channelTitle, titleSha, pubkey, sign, feeI, Messenger::CreateChannelCallback([this, makeFunc, address, channelTitle, titleSha]() {
                LOG << "channel created " << address << " " << channelTitle << " " << titleSha;
                makeFunc(TypedException(), address, channelTitle, titleSha);
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, channelTitle, "");
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::addWriterToChannel(QString address, QString titleSha, QString writer) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgAddWriterToChannelJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &channelSha, const QString &writer) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channelSha, writer);
    };

    const auto errorFunc = [address, titleSha, writer, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, titleSha, writer);
    };

    LOG << "add writer " << address << " " << titleSha << " " << writer;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForChannelAddWriterRequest(titleSha, writer);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, writer, titleSha, makeFunc, errorFunc](const QString &pubkey, const QString &sign) {
            emit messenger->addWriterToChannel(titleSha, writer, pubkey, sign, Messenger::AddWriterToChannelCallback([this, makeFunc, address, titleSha, writer]() {
                LOG << "writer added " << address << " " << titleSha << " " << writer;
                makeFunc(TypedException(), address, titleSha, writer);
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, titleSha, writer);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::delWriterFromChannel(QString address, QString titleSha, QString writer) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgDelWriterFromChannelJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &channelSha, const QString &writer) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channelSha, writer);
    };

    const auto errorFunc = [address, titleSha, writer, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, titleSha, writer);
    };

    LOG << "del writer " << address << " " << titleSha << " " << writer;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForChannelDelWriterRequest(titleSha, writer);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, writer, titleSha, makeFunc, errorFunc](const QString &pubkey, const QString &sign) {
            emit messenger->delWriterFromChannel(titleSha, writer, pubkey, sign, Messenger::DelWriterToChannelCallback([this, makeFunc, address, titleSha, writer]() {
                LOG << "writer deleted " << address << " " << titleSha << " " << writer;
                makeFunc(TypedException(), address, titleSha, writer);
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, titleSha, writer);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::sendMessageToChannel(QString address, QString titleSha, QString dataHex, QString timestampStr, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgMessageSendedToChannelJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &channelSha) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channelSha);
    };

    const auto errorFunc = [address, titleSha, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, titleSha);
    };

    LOG << "sendMessageToChannel " << " " << address << " " << titleSha << " " << timestampStr << " " << feeStr;

    const TypedException exception = apiVrapper2([&, this](){
        CHECK(!dataHex.isEmpty(), "Empty message");
        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "Fee field invalid");
        const uint64_t timestamp = timestampStr.toULongLong(&isValid);
        CHECK(isValid, "timestamp field invalid");

        const QString messageToSign = Messenger::makeTextForSendToChannelRequest(titleSha, dataHex, fee, timestamp);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, dataHex, titleSha, fee, timestamp, makeFunc, errorFunc](const QString &pubkey, const QString &sign) {
            emit messenger->sendMessage(address, address, true, titleSha, dataHex, dataHex, pubkey, sign, fee, timestamp, dataHex, Messenger::SendMessageCallback([this, makeFunc, address, titleSha]() {
                LOG << "Message sended " << address << " " << titleSha;
                makeFunc(TypedException(), address, titleSha);
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, titleSha);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getChannelsList(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetChannelListJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QJsonDocument &channels) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channels);
    };

    const auto errorFunc = [address, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, QJsonDocument());
    };

    LOG << "getChannelList " << " " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getChannelList(address, Messenger::GetChannelListCallback([this, makeFunc, address](const std::vector<ChannelInfo> &channels) {
            LOG << "channel list " << address << " " << channels.size();
            const QJsonDocument channelsJson = channelListToJson(channels);
            makeFunc(TypedException(), address, channelsJson);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getLastMessageChannelNumber(QString address, QString titleSha) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetLastMessageChannelJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, Message::Counter pos) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, pos);
    };

    const auto errorFunc = [address, titleSha, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, titleSha, 0);
    };

    LOG << "get last message channel " << " " << address << " " << titleSha;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getLastMessage(address, true, titleSha, Messenger::GetSavedPosCallback([this, makeFunc, address, titleSha](const Message::Counter &pos) {
            LOG << "Last message number " << address << " " << pos;
            makeFunc(TypedException(), address, titleSha, pos);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, titleSha, 0);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedPosChannel(QString address, QString titleSha) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedPosChannelJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, Message::Counter pos) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, pos);
    };

    const auto errorFunc = [address, titleSha, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, titleSha, 0);
    };

    LOG << "getSavedPosChannel " << address << " " << titleSha;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedPos(address, true, titleSha, Messenger::GetSavedPosCallback([this, makeFunc, address, titleSha](const Message::Counter &pos) {
            LOG << "Saved pos " << address << " " << titleSha << " " << pos;
            makeFunc(TypedException(), address, titleSha, pos);
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, titleSha, 0);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::savePosToChannel(QString address, const QString &titleSha, QString counterStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgStorePosToChannelJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, result);
    };

    const auto errorFunc = [address, titleSha, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, titleSha, "Not ok");
    };

    LOG << "savePosToChannel " << address << " " << titleSha << " " << counterStr;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter counter = counterStr.toLongLong(&isValid);
        CHECK(isValid, "counter field invalid");
        emit messenger->savePos(address, true, titleSha, counter, Messenger::SavePosCallback([this, makeFunc, address, titleSha](){
            LOG << "Save pos ok " << address << " " << titleSha;
            makeFunc(TypedException(), address, titleSha, "Ok");
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, titleSha, "Not ok");
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressChannel(QString address, QString titleSha, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressChannelJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, result);
    };

    const auto errorFunc = [address, titleSha, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, titleSha, QJsonDocument());
    };

    LOG << "get messages channel " << address << " " << titleSha << " " << from << " " << to;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field invalid");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field invalid");
        emit messenger->getHistoryAddressAddress(address, true, titleSha, fromC, toC, Messenger::GetMessagesCallback([this, makeFunc, address, titleSha, errorFunc](const std::vector<Message> &messages) {
            LOG << "Count messages " << address << " " << titleSha << " " << messages.size();
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, titleSha, makeFunc](const std::vector<Message> &messages){
                makeFunc(TypedException(), address, titleSha, messagesToJson(messages));
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, titleSha, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressChannelCount(QString address, QString titleSha, QString count, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressChannelCountJs";

    const auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, result);
    };

    const auto errorFunc = [address, titleSha, makeFunc](const TypedException &exception) {
        makeFunc(exception, address, titleSha, QJsonDocument());
    };

    LOG << "get messagesCC " << address << " " << titleSha << " " << count << " " << to;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter countC = count.toLongLong(&isValid);
        CHECK(isValid, "count field invalid");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field invalid");

        emit messenger->getHistoryAddressAddressCount(address, true, titleSha, countC, toC, Messenger::GetMessagesCallback([this, makeFunc, errorFunc, address, titleSha](const std::vector<Message> &messages) {
            LOG << "Count messagesC " << address << " " << titleSha << " " << messages.size();
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, titleSha, makeFunc](const std::vector<Message> &messages){
                makeFunc(TypedException(), address, titleSha, messagesToJson(messages));
            }, errorFunc, signalFunc));
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeFunc(exception, address, titleSha, QJsonDocument());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::onNewMesseges(QString address, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgNewMessegesJs";

    LOG << "New messages " << lastMessage;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, lastMessage);
END_SLOT_WRAPPER
}

void MessengerJavascript::onAddedToChannel(QString address, QString title, QString titleSha, QString admin, Message::Counter counter) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgAddedToChannelJs";

    LOG << "added to channel " << address << " " << titleSha;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, title, titleSha, admin, counter);
END_SLOT_WRAPPER
}

void MessengerJavascript::onDeletedFromChannel(QString address, QString title, QString titleSha, QString admin) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgDeletedFromChannelJs";

    LOG << "Deleted from channel " << address << " " << titleSha;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, title, titleSha, admin);
END_SLOT_WRAPPER
}

void MessengerJavascript::onNewMessegesChannel(QString address, QString titleSha, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgNewMessegesChannelJs";

    LOG << "New messages channel " << titleSha << " " << lastMessage;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, titleSha, lastMessage);
END_SLOT_WRAPPER
}

void MessengerJavascript::setPathsImpl(QString newPatch, QString /*newUserName*/) {
    CHECK(messenger != nullptr, "Messenger not set");

    LOG << "Set messenger javascript path " << newPatch;

    walletPath = makePath(newPatch, Wallet::WALLET_PATH_MTH);
    CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

    const std::vector<std::pair<QString, QString>> vectors = Wallet::getAllWalletsInFolder(walletPath);
    std::vector<QString> addresses;
    addresses.reserve(vectors.size());
    std::transform(vectors.begin(), vectors.end(), std::back_inserter(addresses), [](const auto &pair) {
        return pair.first;
    });

    emit messenger->addAllAddressesInFolder(walletPath, addresses, Messenger::AddAllWalletsInFolderCallback([](){
        LOG << "addresses added";
    }, [](const TypedException &exception) {
        LOG << "addresses added exception " << exception.numError << " " << exception.description;
    }, signalFunc));
}

void MessengerJavascript::unlockWallet(QString address, QString password, QString passwordRsa, int timeSeconds) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgUnlockWalletResultJs";

    const auto errorFunc = [this, JS_NAME_RESULT, address](const TypedException &exception) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address);
    };

    LOG << "Unlock wallet " << address << " Wallet path " << walletPath;
    const TypedException exception = apiVrapper2([&, this](){
        emit cryptoManager.unlockWallet(walletPath, address, password, passwordRsa, seconds(timeSeconds), CryptographicManager::UnlockWalletCallback([this, address, JS_NAME_RESULT, errorFunc]() {
            emit messenger->decryptMessages(address, Messenger::DecryptUserMessagesCallback([this, address, JS_NAME_RESULT]() {
                makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address);
            }, errorFunc, signalFunc));
         }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::lockWallet() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgLockWalletResultJs";

    const auto errorFunc = [this, JS_NAME_RESULT](const TypedException &exception) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception);
    };

    LOG << "lock wallets";
    const TypedException exception = apiVrapper2([&, this](){
        emit cryptoManager.lockWallet(CryptographicManager::LockWalletCallback([this, JS_NAME_RESULT]() {
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException());
        }, errorFunc, signalFunc));
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::runJs(const QString &script) {
    LOG << "Javascript " << script;
    emit jsRunSig(script);
}

void MessengerJavascript::onLogined(bool isInit, const QString login) {
BEGIN_SLOT_WRAPPER
    if (!login.isEmpty()) {
        setPathsImpl(makePath(defaultWalletPath, login), login);
    } else {
        setPathsImpl(makePath(defaultWalletPath, defaultUserName), defaultUserName);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::onRequiresPubkey(QString address, QString collocutor) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgRequiresPubkeyJs";

    LOG << "Requires pubkey " << address << " " << collocutor;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, collocutor);
END_SLOT_WRAPPER
}

void MessengerJavascript::onCollocutorAddedPubkey(QString address, QString collocutor) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgCollocutorAddedPubkeyJs";

    LOG << "Collocutor added pubkey " << address << " " << collocutor;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, collocutor);
END_SLOT_WRAPPER
}

}
