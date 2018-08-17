#include "MessengerJavascript.h"

#include "check.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "SlotWrapper.h"
#include "utils.h"
#include "Paths.h"

#include "Wallet.h"
#include "WalletRsa.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

MessengerJavascript::MessengerJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &MessengerJavascript::callbackCall, this, &MessengerJavascript::onCallbackCall), "not connect onCallbackCall");
    CHECK(connect(this, &MessengerJavascript::newMessegesSig, this, &MessengerJavascript::onNewMesseges), "not connect onNewMesseges");
    CHECK(connect(this, &MessengerJavascript::addedToChannelSig, this, &MessengerJavascript::onAddedToChannel), "not connect onAddedToChannel");
    CHECK(connect(this, &MessengerJavascript::deletedFromChannelSig, this, &MessengerJavascript::onDeletedFromChannel), "not connect onDeletedFromChannel");
    CHECK(connect(this, &MessengerJavascript::newMessegesChannelSig, this, &MessengerJavascript::onNewMessegesChannel), "not connect onNewMessegesChannel");

    setPaths(getWalletPath(), "");
}

void MessengerJavascript::onCallbackCall(const std::function<void()> &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

template<typename... Args>
void MessengerJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc2<false>(function, "", exception, std::forward<Args>(args)...);
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

    const Message::Counter fromC = std::stoll(from.toStdString());
    const Message::Counter toC = std::stoll(to.toStdString());
    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getHistoryAddress(address, fromC, toC, [this, JS_NAME_RESULT, address](const std::vector<Message> &messages, const TypedException &exception) {
            Opt<QJsonDocument> result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messages " << address << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception2, Opt<QString>(address), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QJsonDocument>());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddress(QString address, QString collocutor, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressJs";

    LOG << "get messages " << address << " " << collocutor << " " << from << " " << to;

    const Message::Counter fromC = std::stoll(from.toStdString());
    const Message::Counter toC = std::stoll(to.toStdString());
    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getHistoryAddressAddress(address, false, collocutor, fromC, toC, [this, JS_NAME_RESULT, address, collocutor](const std::vector<Message> &messages, const TypedException &exception) {
            Opt<QJsonDocument> result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messages " << address << " " << collocutor << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception2, Opt<QString>(address), Opt<QString>(collocutor), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), Opt<QJsonDocument>());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddressCount(QString address, QString collocutor, QString count, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressCountJs";

    LOG << "get messagesC " << address << " " << collocutor << " " << count << " " << to;

    const Message::Counter countC = std::stoll(count.toStdString());
    const Message::Counter toC = std::stoll(to.toStdString());
    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getHistoryAddressAddressCount(address, false, collocutor, countC, toC, [this, JS_NAME_RESULT, address, collocutor](const std::vector<Message> &messages, const TypedException &exception) {
            Opt<QJsonDocument> result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messagesC " << address << " " << collocutor << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception2, Opt<QString>(address), Opt<QString>(collocutor), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), Opt<QJsonDocument>());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::registerAddress(bool isForcibly, QString address, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgAddressAppendToMessengerJs";

    LOG << "registerAddress " << address;

    const uint64_t fee = std::stoull(feeStr.toStdString());
    const TypedException exception = apiVrapper2([&, this](){
        const QString pubkeyRsa = QString::fromStdString(walletManager.getWalletRsa(address.toStdString()).getPublikKey());
        const QString messageToSign = Messenger::makeTextForSignRegisterRequest(address, pubkeyRsa, fee);
        std::string pubkey;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pubkey);
        emit messenger->registerAddress(isForcibly, address, pubkeyRsa, QString::fromStdString(pubkey), QString::fromStdString(sign), fee, [this, JS_NAME_RESULT, address, isForcibly](bool isNew, const TypedException &exception) {
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
                    emit messenger->signedStrings(address, result, [this, JS_NAME_RESULT, address](const TypedException &exception){
                        LOG << "Address registered " << address;
                        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>("Ok"));
                    });
                }
            });
            if (exception2.isSet()) {
                makeAndRunJsFuncParams(JS_NAME_RESULT, exception2, Opt<QString>("Not ok"));
            }
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>("Not ok"));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::savePublicKeyCollocutor(bool isForcibly, QString address, QString collocutor) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgPublicKeyCollocutorGettedJs";

    LOG << "savePublicKeyCollocutor " << address << " " << collocutor;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForGetPubkeyRequest(collocutor);
        std::string pubkey;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pubkey);

        emit messenger->savePubkeyAddress(isForcibly, collocutor, QString::fromStdString(pubkey), QString::fromStdString(sign), [this, JS_NAME_RESULT, address, collocutor](bool /*isNew*/, const TypedException &exception) {
            LOG << "Pubkey saved " << collocutor;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::sendMessage(QString address, QString collocutor, QString dataHex, QString timestampStr, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgMessageSendedJs";

    LOG << "sendMessage " << " " << address << " " << collocutor << " " << timestampStr << " " << feeStr;

    const TypedException exception = apiVrapper2([&, this](){
        const uint64_t fee = std::stoull(feeStr.toStdString());
        const uint64_t timestamp = std::stoull(timestampStr.toStdString());
        emit messenger->getPubkeyAddress(collocutor, [this, JS_NAME_RESULT, address, collocutor, dataHex, fee, timestamp](const QString &pubkey, const TypedException &exception) {
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
                emit messenger->sendMessage(address, collocutor, false, "", encryptedDataToWss, QString::fromStdString(pub), QString::fromStdString(sign), fee, timestamp, encryptedDataToBd, [this, JS_NAME_RESULT, address, collocutor](const TypedException &exception) {
                    LOG << "Message sended " << address << " " << collocutor;
                    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor));
                });
            });
            if (exception2.isSet()) {
                makeAndRunJsFuncParams(JS_NAME_RESULT, exception2, Opt<QString>(address), Opt<QString>(collocutor));
            }
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getLastMessageNumber(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgLastMessegesJs";

    LOG << "getLastMessageNumber " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getLastMessage(address, false, "", [this, JS_NAME_RESULT, address](const Message::Counter &pos, const TypedException &exception) {
            LOG << "Last message number " << address << " " << pos;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<Message::Counter>(pos));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<Message::Counter>(0));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedPos(QString address, const QString &collocutor) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedPosJs";

    LOG << "getSavedPos " << address << " " << collocutor;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedPos(address, false, collocutor, [this, JS_NAME_RESULT, address, collocutor](const Message::Counter &pos, const TypedException &exception) {
            LOG << "Saved pos " << address << " " << collocutor << " " << pos;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), Opt<Message::Counter>(pos));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), Opt<Message::Counter>(0));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedsPos(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedsPosJs";

    LOG << "getSavedsPos " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedsPos(address, false, [this, JS_NAME_RESULT, address](const std::vector<std::pair<QString, Message::Counter>> &pos, const TypedException &exception) {
            const Opt<QJsonDocument> result(allPosToJson(pos));

            LOG << "Get saveds pos " << pos.size();
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::savePos(QString address, const QString &collocutor, QString counterStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgStorePosJs";

    LOG << "savePos " << address << " " << collocutor << " " << counterStr;

    const TypedException exception = apiVrapper2([&, this](){
        const Message::Counter counter = std::stoll(counterStr.toStdString());
        emit messenger->savePos(address, false, collocutor, counter, [this, JS_NAME_RESULT, address, collocutor](const TypedException &exception){
            LOG << "Save pos ok " << address << " " << collocutor;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), Opt<QString>("Ok"));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), Opt<QString>("Not ok"));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getCountMessages(QString address, const QString &collocutor, QString from) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgCountMessagesJs";

    LOG << "getCountMessages " << address << " " << collocutor << " " << from;

    const TypedException exception = apiVrapper2([&, this](){
        const Message::Counter fromI = std::stoll(from.toStdString());
        emit messenger->getCountMessages(address, collocutor, fromI, [this, JS_NAME_RESULT, address, collocutor, fromI](const Message::Counter &count, const TypedException &exception) {
            LOG << "Count messages " << address << " " << collocutor << " " << fromI << " " << count;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), Opt<Message::Counter>(count));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), Opt<Message::Counter>(0));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::createChannel(QString address, QString channelTitle, QString fee) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgChannelCreateJs";

    LOG << "channel create " << address << " " << channelTitle << " " << fee;

    const TypedException exception = apiVrapper2([&, this](){
        const Message::Counter feeI = std::stoull(fee.toStdString());
        const QString titleSha = Messenger::getChannelSha(channelTitle);
        const QString messageToSign = Messenger::makeTextForChannelCreateRequest(channelTitle, titleSha, feeI);
        std::string pub;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub);

        emit messenger->createChannel(channelTitle, titleSha, QString::fromStdString(pub), QString::fromStdString(sign), feeI, [this, JS_NAME_RESULT, address, channelTitle, titleSha](const TypedException &exception) {
            LOG << "channel created " << address << " " << channelTitle << " " << titleSha;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(channelTitle), Opt<QString>(titleSha));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(channelTitle), Opt<QString>());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::addWriterToChannel(QString address, QString titleSha, QString writer) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgAddWriterToChannelJs";

    LOG << "add writer " << address << " " << titleSha << " " << writer;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForChannelAddWriterRequest(titleSha, writer);
        std::string pub;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub);

        emit messenger->addWriterToChannel(titleSha, writer, QString::fromStdString(pub), QString::fromStdString(sign), [this, JS_NAME_RESULT, address, titleSha, writer](const TypedException &exception) {
            LOG << "writer added " << address << " " << titleSha << " " << writer;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<QString>(writer));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<QString>(writer));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::delWriterFromChannel(QString address, QString titleSha, QString writer) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgDelWriterFromChannelJs";

    LOG << "del writer " << address << " " << titleSha << " " << writer;

    const TypedException exception = apiVrapper2([&, this](){
        const QString messageToSign = Messenger::makeTextForChannelDelWriterRequest(titleSha, writer);
        std::string pub;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub); // TODO подумать вынести эти 3 строки в отдельную функцию

        emit messenger->delWriterFromChannel(titleSha, writer, QString::fromStdString(pub), QString::fromStdString(sign), [this, JS_NAME_RESULT, address, titleSha, writer](const TypedException &exception) {
            LOG << "writer deleted " << address << " " << titleSha << " " << writer;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<QString>(writer));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<QString>(writer));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::sendMessageToChannel(QString address, QString titleSha, QString dataHex, QString timestampStr, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgMessageSendedToChannelJs";

    LOG << "sendMessageToChannel " << " " << address << " " << titleSha << " " << timestampStr << " " << feeStr;

    const TypedException exception = apiVrapper2([&, this](){
        const uint64_t fee = std::stoull(feeStr.toStdString());
        const uint64_t timestamp = std::stoull(timestampStr.toStdString());

        const QString messageToSign = Messenger::makeTextForSendToChannelRequest(titleSha, dataHex, fee, timestamp);
        std::string pub;
        const std::string &sign = walletManager.getWallet(address.toStdString()).sign(messageToSign.toStdString(), pub);

        emit messenger->sendMessage(address, address, true, titleSha, dataHex, QString::fromStdString(pub), QString::fromStdString(sign), fee, timestamp, dataHex, [this, JS_NAME_RESULT, address, titleSha](const TypedException &exception) {
            LOG << "Message sended " << address << " " << titleSha;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getChannelsList(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetChannelListJs";

    LOG << "getChannelList " << " " << address;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getChannelList(address, [this, JS_NAME_RESULT, address](const std::vector<ChannelInfo> &channels, const TypedException &exception) {
            LOG << "channel list " << address << " " << channels.size();
            const QJsonDocument channelsJson = channelListToJson(channels);
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QJsonDocument>(channelsJson));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QJsonDocument>());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getLastMessageChannelNumber(QString address, QString titleSha) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetLastMessageChannelJs";

    LOG << "get last message channel " << " " << address << " " << titleSha;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getLastMessage(address, true, titleSha, [this, JS_NAME_RESULT, address, titleSha](const Message::Counter &pos, const TypedException &exception) {
            LOG << "Last message number " << address << " " << pos;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<Message::Counter>(pos));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<Message::Counter>(0));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedPosChannel(QString address, QString titleSha) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedPosChannelJs";

    LOG << "getSavedPosChannel " << address << " " << titleSha;

    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getSavedPos(address, true, titleSha, [this, JS_NAME_RESULT, address, titleSha](const Message::Counter &pos, const TypedException &exception) {
            LOG << "Saved pos " << address << " " << titleSha << " " << pos;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<Message::Counter>(pos));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<Message::Counter>(0));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::savePosToChannel(QString address, const QString &titleSha, QString counterStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgStorePosToChannelJs";

    LOG << "savePosToChannel " << address << " " << titleSha << " " << counterStr;

    const TypedException exception = apiVrapper2([&, this](){
        const Message::Counter counter = std::stoll(counterStr.toStdString());
        emit messenger->savePos(address, true, titleSha, counter, [this, JS_NAME_RESULT, address, titleSha](const TypedException &exception){
            LOG << "Save pos ok " << address << " " << titleSha;
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<QString>("Ok"));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<QString>("Not ok"));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressChannel(QString address, QString titleSha, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressChannelJs";

    LOG << "get messages channel " << address << " " << titleSha << " " << from << " " << to;

    const Message::Counter fromC = std::stoll(from.toStdString());
    const Message::Counter toC = std::stoll(to.toStdString());
    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getHistoryAddressAddress(address, true, titleSha, fromC, toC, [this, JS_NAME_RESULT, address, titleSha](const std::vector<Message> &messages, const TypedException &exception) {
            Opt<QJsonDocument> result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messages " << address << " " << titleSha << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception2, Opt<QString>(address), Opt<QString>(titleSha), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<QJsonDocument>());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressChannelCount(QString address, QString titleSha, QString count, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressChannelCountJs";

    LOG << "get messagesCC " << address << " " << titleSha << " " << count << " " << to;

    const Message::Counter countC = std::stoll(count.toStdString());
    const Message::Counter toC = std::stoll(to.toStdString());
    const TypedException exception = apiVrapper2([&, this](){
        emit messenger->getHistoryAddressAddressCount(address, true, titleSha, countC, toC, [this, JS_NAME_RESULT, address, titleSha](const std::vector<Message> &messages, const TypedException &exception) {
            Opt<QJsonDocument> result;
            const TypedException exception2 = apiVrapper2([&, this](){
                if (exception.isSet()) {
                    throw exception;
                }

                LOG << "Count messagesC " << address << " " << titleSha << " " << messages.size();
                result = messagesToJson(messages, walletManager.getWalletRsa(address.toStdString()));
            });
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception2, Opt<QString>(address), Opt<QString>(titleSha), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(titleSha), Opt<QJsonDocument>());
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::onNewMesseges(QString address, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgNewMessegesJs";

    LOG << "New messages " << lastMessage;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<Message::Counter>(lastMessage));
END_SLOT_WRAPPER
}

void MessengerJavascript::onAddedToChannel(QString address, QString title, QString titleSha, QString admin, Message::Counter counter) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgAddedToChannelJs";

    LOG << "added to channel " << address << " " << titleSha;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(title), Opt<QString>(titleSha), Opt<QString>(admin), Opt<Message::Counter>(counter));
END_SLOT_WRAPPER
}

void MessengerJavascript::onDeletedFromChannel(QString address, QString title, QString titleSha, QString admin) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgDeletedFromChannelJs";

    LOG << "Deleted from channel " << address << " " << titleSha;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(title), Opt<QString>(titleSha), Opt<QString>(admin));
END_SLOT_WRAPPER
}

void MessengerJavascript::onNewMessegesChannel(QString address, QString titleSha, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgNewMessegesChannelJs";

    LOG << "New messages channel " << titleSha << " " << lastMessage;

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(titleSha), Opt<Message::Counter>(lastMessage));
END_SLOT_WRAPPER
}

void MessengerJavascript::setPaths(QString newPatch, QString /*newUserName*/) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "setMessengerPathsJs";

    LOG << "Set messenger javascript path " << newPatch;

    Opt<QString> result;
    const TypedException exception = apiVrapper2([&, this]() {
        walletPath = makePath(newPatch, Wallet::WALLET_PATH_MTH);
        CHECK(!walletPath.isNull() && !walletPath.isEmpty(), "Incorrect path to wallet: empty");

        result = "Ok";
    });

    if (exception.isSet()) {
        result = "Not ok";
    }
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, result);
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
