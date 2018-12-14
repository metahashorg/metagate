#include "MessengerJavascript.h"

#include "check.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "SlotWrapper.h"
#include "utils.h"
#include "Paths.h"

#include "Messenger.h"

#include "Wallet.h"
#include "WalletRsa.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "auth/Auth.h"
#include "JavascriptWrapper.h"

namespace messenger {

MessengerJavascript::MessengerJavascript(auth::Auth &authManager, const JavascriptWrapper &jManager, QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &MessengerJavascript::callbackCall, this, &MessengerJavascript::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &MessengerJavascript::newMessegesSig, this, &MessengerJavascript::onNewMesseges), "not connect onNewMesseges");
    CHECK(connect(this, &MessengerJavascript::addedToChannelSig, this, &MessengerJavascript::onAddedToChannel), "not connect onAddedToChannel");
    CHECK(connect(this, &MessengerJavascript::deletedFromChannelSig, this, &MessengerJavascript::onDeletedFromChannel), "not connect onDeletedFromChannel");
    CHECK(connect(this, &MessengerJavascript::newMessegesChannelSig, this, &MessengerJavascript::onNewMessegesChannel), "not connect onNewMessegesChannel");

    CHECK(connect(&authManager, &auth::Auth::logined, this, &MessengerJavascript::onLogined), "not connect onLogined");

    qRegisterMetaType<Callback>("Callback");

    defaultWalletPath = jManager.walletDefaultPath;
    defaultUserName = jManager.defaultUsername;

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

static QJsonDocument messagesToJson(const std::vector<Message> &messages, const WalletRsa &walletRsa) {
    QJsonArray messagesArrJson;
    for (const Message &message: messages) {
        QJsonObject messageJson;

        const bool isEncrypted = !message.isChannel;

        messageJson.insert("collocutor", message.collocutor);
        messageJson.insert("isInput", message.isInput);
        messageJson.insert("timestamp", QString::fromStdString(std::to_string(message.timestamp)));
        if (!isEncrypted) {
            messageJson.insert("data", message.data);
        } else {
            if (message.isCanDecrypted) {
                const std::string decryptedData = toHex(walletRsa.decryptMessage(message.data.toStdString()));
                messageJson.insert("data", QString::fromStdString(decryptedData));
            }
        }
        messageJson.insert("isDecrypter", message.isCanDecrypted || !isEncrypted);
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddress(address, fromC, toC, [this, address, makeFunc](const std::vector<Message> &messages, const TypedException &exception) {
            QJsonDocument result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messages " << address << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeFunc(exception2, address, result);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, result);
    };

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddressAddress(address, false, collocutor, fromC, toC, [this, address, collocutor, makeFunc](const std::vector<Message> &messages, const TypedException &exception) {
            QJsonDocument result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messages " << address << " " << collocutor << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeFunc(exception2, address, collocutor, result);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, result);
    };

    LOG << "get messagesC " << address << " " << collocutor << " " << count << " " << to;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter countC = count.toLongLong(&isValid);
        CHECK(isValid, "count field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddressAddressCount(address, false, collocutor, countC, toC, [this, address, collocutor, makeFunc](const std::vector<Message> &messages, const TypedException &exception) {
            QJsonDocument result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messagesC " << address << " " << collocutor << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeFunc(exception2, address, collocutor, result);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
    };

    LOG << "registerAddress " << address;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "Fee field incorrect");
        const QString pubkeyRsa = QString::fromStdString(walletManager.getWalletRsa(address.toStdString()).getPublikKey());
        const QString messageToSign = Messenger::makeTextForSignRegisterRequest(address, pubkeyRsa, fee);
        std::string pubkey;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pubkey);
        emit messenger->registerAddress(isForcibly, address, pubkeyRsa, QString::fromStdString(pubkey), QString::fromStdString(sign), fee, [this, address, isForcibly, makeFunc](bool isNew, const TypedException &exception) mutable {
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet() && !isForcibly) {
                    throw exception;
                }

                if (isNew || isForcibly) {
                    const std::vector<QString> messagesForSign = Messenger::stringsForSign();
                    std::vector<QString> result;
                    for (const QString &msg: messagesForSign) {
                        std::string tmp;
                        const std::string sign = walletManager.getWallet(address.toStdString()).sign(msg.toStdString(), tmp);
                        result.emplace_back(QString::fromStdString(sign));
                    }
                    emit messenger->signedStrings(address, result, [this, address, makeFunc](const TypedException &exception){
                        LOG << "Address registered " << address;
                        makeFunc(exception, QString("Ok"));
                    });
                }
            });
            if (exception2.isSet()) {
                makeFunc(exception2, QString("Not ok"));
            }
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor);
    };

    LOG << "savePublicKeyCollocutor " << address << " " << collocutor;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForGetPubkeyRequest(collocutor);
        std::string pubkey;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pubkey);

        emit messenger->savePubkeyAddress(isForcibly, collocutor, QString::fromStdString(pubkey), QString::fromStdString(sign), [this, address, collocutor, makeFunc](bool /*isNew*/, const TypedException &exception) {
            LOG << "Pubkey saved " << collocutor;
            makeFunc(exception, address, collocutor);
        });
    });

    if (exception.isSet()) {
        makeFunc(exception, address, collocutor);
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

    LOG << "sendMessage " << " " << address << " " << collocutor << " " << timestampStr << " " << feeStr;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "fee field incorrect");
        const uint64_t timestamp = timestampStr.toULongLong(&isValid);
        CHECK(isValid, "timestamp field incorrect");
        emit messenger->getPubkeyAddress(collocutor, [this, makeFunc, address, collocutor, dataHex, fee, timestamp](const QString &pubkey, const TypedException &exception) mutable {
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                const std::string data = fromHex(dataHex.toStdString());
                const WalletRsa walletRsa = WalletRsa::fromPublicKey(pubkey.toStdString());
                const QString encryptedDataToWss = QString::fromStdString(walletRsa.encrypt(data));

                const QString messageToSign = Messenger::makeTextForSendMessageRequest(collocutor, encryptedDataToWss, fee, timestamp);
                std::string pub;
                const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub);

                const QString encryptedDataToBd = QString::fromStdString(walletManager.getWalletRsa(address.toStdString()).encrypt(data));
                emit messenger->sendMessage(address, collocutor, false, "", encryptedDataToWss, QString::fromStdString(pub), QString::fromStdString(sign), fee, timestamp, encryptedDataToBd, [this, makeFunc, address, collocutor](const TypedException &exception) {
                    LOG << "Message sended " << address << " " << collocutor;
                    makeFunc(exception, address, collocutor);
                });
            });
            if (exception2.isSet()) {
                makeFunc(exception2, address, collocutor);
            }
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const Message::Counter &pos) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, pos);
    };

    LOG << "getLastMessageNumber " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getLastMessage(address, false, "", [this, makeFunc, address](const Message::Counter &pos, const TypedException &exception) {
            LOG << "Last message number " << address << " " << pos;
            makeFunc(exception, address, pos);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const Message::Counter &pos) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, pos);
    };

    LOG << "getSavedPos " << address << " " << collocutor;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedPos(address, false, collocutor, [this, makeFunc, address, collocutor](const Message::Counter &pos, const TypedException &exception) {
            LOG << "Saved pos " << address << " " << collocutor << " " << pos;
            makeFunc(exception, address, collocutor, pos);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, result);
    };

    LOG << "getSavedsPos " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedsPos(address, false, [this, makeFunc, address](const std::vector<std::pair<QString, Message::Counter>> &pos, const TypedException &exception) {
            const QJsonDocument result(allPosToJson(pos));

            LOG << "Get saveds pos " << pos.size();
            makeFunc(exception, address, result);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, result);
    };

    LOG << "savePos " << address << " " << collocutor << " " << counterStr;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter counter = counterStr.toLongLong(&isValid);
        CHECK(isValid, "counter field invalid");
        emit messenger->savePos(address, false, collocutor, counter, [this, makeFunc, address, collocutor](const TypedException &exception){
            LOG << "Save pos ok " << address << " " << collocutor;
            makeFunc(exception, address, collocutor, "Ok");
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &collocutor, const Message::Counter &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, collocutor, result);
    };

    LOG << "getCountMessages " << address << " " << collocutor << " " << from;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter fromI = from.toLongLong(&isValid);
        CHECK(isValid, "from field invalid");
        emit messenger->getCountMessages(address, collocutor, fromI, [this, makeFunc, address, collocutor, fromI](const Message::Counter &count, const TypedException &exception) {
            LOG << "Count messages " << address << " " << collocutor << " " << fromI << " " << count;
            makeFunc(exception, address, collocutor, count);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &channel, const QString &channelSha) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channel, channelSha);
    };

    LOG << "channel create " << address << " " << channelTitle << " " << fee;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter feeI = fee.toLongLong(&isValid);
        CHECK(isValid, "fee field invalid");
        const QString titleSha = Messenger::getChannelSha(channelTitle);
        const QString messageToSign = Messenger::makeTextForChannelCreateRequest(channelTitle, titleSha, feeI);
        std::string pub;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub);

        emit messenger->createChannel(address, channelTitle, titleSha, QString::fromStdString(pub), QString::fromStdString(sign), feeI, [this, makeFunc, address, channelTitle, titleSha](const TypedException &exception) {
            LOG << "channel created " << address << " " << channelTitle << " " << titleSha;
            makeFunc(exception, address, channelTitle, titleSha);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &channelSha, const QString &writer) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channelSha, writer);
    };

    LOG << "add writer " << address << " " << titleSha << " " << writer;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForChannelAddWriterRequest(titleSha, writer);
        std::string pub;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub);

        emit messenger->addWriterToChannel(titleSha, writer, QString::fromStdString(pub), QString::fromStdString(sign), [this, makeFunc, address, titleSha, writer](const TypedException &exception) {
            LOG << "writer added " << address << " " << titleSha << " " << writer;
            makeFunc(exception, address, titleSha, writer);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &channelSha, const QString &writer) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channelSha, writer);
    };

    LOG << "del writer " << address << " " << titleSha << " " << writer;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForChannelDelWriterRequest(titleSha, writer);
        std::string pub;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub); // TODO подумать вынести эти 3 строки в отдельную функцию

        emit messenger->delWriterFromChannel(titleSha, writer, QString::fromStdString(pub), QString::fromStdString(sign), [this, makeFunc, address, titleSha, writer](const TypedException &exception) {
            LOG << "writer deleted " << address << " " << titleSha << " " << writer;
            makeFunc(exception, address, titleSha, writer);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &channelSha) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channelSha);
    };

    LOG << "sendMessageToChannel " << " " << address << " " << titleSha << " " << timestampStr << " " << feeStr;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "Fee field invalid");
        const uint64_t timestamp = timestampStr.toULongLong(&isValid);
        CHECK(isValid, "timestamp field invalid");

        const QString messageToSign = Messenger::makeTextForSendToChannelRequest(titleSha, dataHex, fee, timestamp);
        std::string pub;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub);

        emit messenger->sendMessage(address, address, true, titleSha, dataHex, QString::fromStdString(pub), QString::fromStdString(sign), fee, timestamp, dataHex, [this, makeFunc, address, titleSha](const TypedException &exception) {
            LOG << "Message sended " << address << " " << titleSha;
            makeFunc(exception, address, titleSha);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QJsonDocument &channels) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, channels);
    };

    LOG << "getChannelList " << " " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getChannelList(address, [this, makeFunc, address](const std::vector<ChannelInfo> &channels, const TypedException &exception) {
            LOG << "channel list " << address << " " << channels.size();
            const QJsonDocument channelsJson = channelListToJson(channels);
            makeFunc(exception, address, channelsJson);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, Message::Counter pos) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, pos);
    };

    LOG << "get last message channel " << " " << address << " " << titleSha;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getLastMessage(address, true, titleSha, [this, makeFunc, address, titleSha](const Message::Counter &pos, const TypedException &exception) {
            LOG << "Last message number " << address << " " << pos;
            makeFunc(exception, address, titleSha, pos);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, Message::Counter pos) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, pos);
    };

    LOG << "getSavedPosChannel " << address << " " << titleSha;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedPos(address, true, titleSha, [this, makeFunc, address, titleSha](const Message::Counter &pos, const TypedException &exception) {
            LOG << "Saved pos " << address << " " << titleSha << " " << pos;
            makeFunc(exception, address, titleSha, pos);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, const QString &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, result);
    };

    LOG << "savePosToChannel " << address << " " << titleSha << " " << counterStr;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter counter = counterStr.toLongLong(&isValid);
        CHECK(isValid, "counter field invalid");
        emit messenger->savePos(address, true, titleSha, counter, [this, makeFunc, address, titleSha](const TypedException &exception){
            LOG << "Save pos ok " << address << " " << titleSha;
            makeFunc(exception, address, titleSha, "Ok");
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, result);
    };

    LOG << "get messages channel " << address << " " << titleSha << " " << from << " " << to;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field invalid");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field invalid");
        emit messenger->getHistoryAddressAddress(address, true, titleSha, fromC, toC, [this, makeFunc, address, titleSha](const std::vector<Message> &messages, const TypedException &exception) {
            QJsonDocument result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messages " << address << " " << titleSha << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeFunc(exception2, address, titleSha, result);
        });
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

    auto makeFunc = [JS_NAME_RESULT, this](const TypedException &exception, const QString &address, const QString &titleSha, const QJsonDocument &result) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, address, titleSha, result);
    };

    LOG << "get messagesCC " << address << " " << titleSha << " " << count << " " << to;

    const TypedException exception = apiVrapper2([&, this](){
        bool isValid;
        const Message::Counter countC = count.toLongLong(&isValid);
        CHECK(isValid, "count field invalid");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field invalid");

        emit messenger->getHistoryAddressAddressCount(address, true, titleSha, countC, toC, [this, makeFunc, address, titleSha](const std::vector<Message> &messages, const TypedException &exception) {
            QJsonDocument result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messagesC " << address << " " << titleSha << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeFunc(exception2, address, titleSha, result);
        });
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
    LOG << "Set messenger javascript path " << newPatch;

    walletPath = makePath(newPatch, Wallet::WALLET_PATH_MTH);
    CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");
}

void MessengerJavascript::setPaths(QString newPatch, QString newUserName) {
BEGIN_SLOT_WRAPPER
    /*const QString JS_NAME_RESULT = "setMessengerPathsJs";
    QString result;
    const TypedException exception = apiVrapper2([&, this]() {
        setPathsImpl(newPatch, newUserName);

        result = "Ok";
    });

    if (exception.isSet()) {
        result = "Not ok";
    }
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);*/
END_SLOT_WRAPPER
}

void MessengerJavascript::unlockWallet(QString address, QString password, QString passwordRsa, int timeSeconds) {
    LOG << "Unlock wallet " << address << " Wallet path " << walletPath;
    walletManager.unlockWallet(walletPath, address.toStdString(), password.toStdString(), passwordRsa.toStdString(), seconds(timeSeconds));
}

void MessengerJavascript::lockWallet() {
    LOG << "lock wallets";
    walletManager.lockWallet();
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

}
