#include "MessengerJavascript.h"

#include "check.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "SlotWrapper.h"
#include "utilites/utils.h"
#include "Paths.h"
#include "QRegister.h"

#include "CryptographicManager.h"
#include "Messenger.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

#include "Wallet.h"

#include "auth/Auth.h"
#include "transactions/Transactions.h"
#include "JavascriptWrapper.h"

#include "WrapperJavascriptImpl.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("MSG");

namespace messenger {

MessengerJavascript::MessengerJavascript(auth::Auth &authManager, CryptographicManager &cryptoManager, transactions::Transactions &txManager, JavascriptWrapper &jsWrapper, QObject *parent)
    : WrapperJavascript(false, LOG_FILE)
    , authManager(authManager)
    , cryptoManager(cryptoManager)
    , txManager(txManager)
{
    Q_CONNECT(this, &MessengerJavascript::newMessegesSig, this, &MessengerJavascript::onNewMesseges);
    Q_CONNECT(this, &MessengerJavascript::addedToChannelSig, this, &MessengerJavascript::onAddedToChannel);
    Q_CONNECT(this, &MessengerJavascript::deletedFromChannelSig, this, &MessengerJavascript::onDeletedFromChannel);
    Q_CONNECT(this, &MessengerJavascript::newMessegesChannelSig, this, &MessengerJavascript::onNewMessegesChannel);
    Q_CONNECT(this, &MessengerJavascript::requiresPubkeySig, this, &MessengerJavascript::onRequiresPubkey);
    Q_CONNECT(this, &MessengerJavascript::collocutorAddedPubkeySig, this, &MessengerJavascript::onCollocutorAddedPubkey);

    Q_CONNECT(&authManager, &auth::Auth::logined, this, &MessengerJavascript::onLogined);
    Q_CONNECT(&jsWrapper, &JavascriptWrapper::mthWalletCreated, this, &MessengerJavascript::onMthWalletCreated);

    walletPath = getWalletPath();
    mthWalletType = Wallet::WALLET_PATH_MTH;
    defaultUserName = JavascriptWrapper::defaultUsername;

    emit authManager.reEmit();
}

void MessengerJavascript::setMessenger(Messenger &m) {
    messenger = &m;
    emit authManager.reEmit();
    setPathsImpl();
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

static QJsonDocument contactInfoToJson(bool complete, const ContactInfo &info) {
    QJsonObject result;
    result.insert("pubkey", info.pubkeyRsa);
    result.insert("txHash", info.txRsaHash);
    result.insert("blockchain_name", info.blockchainName);
    result.insert("is_complete", complete);
    return QJsonDocument(result);
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

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddress(address, fromC, toC, Messenger::GetMessagesCallback([this, address, makeFunc](const std::vector<Message> &messages) {
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, makeFunc](const std::vector<Message> &messages){
                LOG << "get messages ok " << address << " " << messages.size();
                makeFunc.func(TypedException(), address, messagesToJson(messages));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddress(QString address, QString collocutor, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressJs";

    LOG << "get messages " << address << " " << collocutor << " " << from << " " << to;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(collocutor), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddressAddress(address, false, collocutor, fromC, toC, Messenger::GetMessagesCallback([this, address, collocutor, makeFunc](const std::vector<Message> &messages) {
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, collocutor, makeFunc](const std::vector<Message> &messages){
                LOG << "get messages ok " << address << " " << collocutor << " " << messages.size();
                makeFunc.func(TypedException(), address, collocutor, messagesToJson(messages));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddressCount(QString address, QString collocutor, QString count, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressCountJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(collocutor), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    LOG << "get messagesC " << address << " " << collocutor << " " << count << " " << to;

    wrapOperation([&, this](){
        bool isValid;
        const Message::Counter countC = count.toLongLong(&isValid);
        CHECK(isValid, "count field incorrect");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field incorrect");

        emit messenger->getHistoryAddressAddressCount(address, false, collocutor, countC, toC, Messenger::GetMessagesCallback([this, address, collocutor, makeFunc](const std::vector<Message> &messages) {
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, collocutor, makeFunc](const std::vector<Message> &messages){
                LOG << "get messagesC ok " << address << " " << collocutor << " " << messages.size();
                makeFunc.func(TypedException(), address, collocutor, messagesToJson(messages));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::sendPubkeyAddressToBlockchain(QString address, QString feeStr, QString paramsJson) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");
    const QString JS_NAME_RESULT = "msgPubkeyAddressToBlockchainResultJs";

    LOG << "Send pubkey to blockchain " << address << " " << feeStr << " " << paramsJson;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    wrapOperation([&, this]{
        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "Fee field incorrect");

        const transactions::SendParameters sendParams = transactions::parseSendParams(paramsJson);
        emit cryptoManager.getPubkeyRsa(address, CryptographicManager::GetPubkeyRsaCallback([this, address, fee, sendParams, makeFunc](const QString &pubkeyRsa){
            emit txManager.getNonce("1", address, sendParams, transactions::Transactions::GetNonceCallback([this, address, fee, sendParams, pubkeyRsa, makeFunc](size_t nonce, const QString &server) {
                LOG << "Get nonce ok " << nonce;
                emit cryptoManager.signTransaction(address, address, 0, fee, nonce, pubkeyRsa, CryptographicManager::SignTransactionCallback([this, address, pubkeyRsa, nonce, fee, sendParams, makeFunc](const QString &transaction, const QString &pubkey, const QString &sign){
                    LOG << "Sign Transaction size " << transaction.size();
                    const QString feeStr = QString::fromStdString(std::to_string(fee));
                    emit txManager.sendTransaction("1", address, "0", nonce, pubkeyRsa, feeStr, pubkey, sign, sendParams, transactions::Transactions::SendTransactionCallback([address, makeFunc](){
                        LOG << "Send pubkey to blockchain ok " << address;
                        makeFunc.func(TypedException(), "Ok");
                    }, makeFunc.error, signalFunc));
                }, makeFunc.error, signalFunc));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::doRegisterAddress(const QString &address, bool isNew, bool isForcibly, const std::function<void(const TypedException &exception, const QString &result)> &makeFunc, const std::function<void(const TypedException &exception)> &errorFunc) {
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
}

void MessengerJavascript::registerAddress(bool isForcibly, QString address, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgAddressAppendToMessengerJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    LOG << "registerAddress " << address << " " << isForcibly;

    wrapOperation([&, this](){
        const auto processFunc = std::bind(&MessengerJavascript::doRegisterAddress, this, address, _1, isForcibly, makeFunc.func, makeFunc.error);

        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "Fee field incorrect");
        emit cryptoManager.getPubkeyRsa(address, CryptographicManager::GetPubkeyRsaCallback([this, address, isForcibly, fee, makeFunc, processFunc](const QString &pubkeyRsa){
            const QString messageToSign = Messenger::makeTextForSignRegisterRequest(address, pubkeyRsa, fee);
            emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, isForcibly, fee, makeFunc, processFunc, pubkeyRsa](const QString &pubkey, const QString &sign){
                emit messenger->registerAddress(isForcibly, address, pubkeyRsa, pubkey, sign, fee, Messenger::RegisterAddressCallback(processFunc, makeFunc.error, signalFunc));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::registerAddressBlockchain(bool isForcibly, QString address, QString feeStr, QString txHash, QString blockchainName, QString blockchainServ) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgAddressAppendToMessengerBlockchainJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>("Not ok"));

    LOG << "registerAddressBlockchain " << address << " " << isForcibly << " " << txHash << " " << blockchainName << " " << blockchainServ;

    wrapOperation([&, this](){
        const auto processFunc = std::bind(&MessengerJavascript::doRegisterAddress, this, address, _1, isForcibly, makeFunc.func, makeFunc.error);

        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "Fee field incorrect");
        const QString messageToSign = Messenger::makeTextForSignRegisterBlockchainRequest(address, fee, txHash, blockchainServ, blockchainName);
        emit cryptoManager.getPubkeyRsa(address, CryptographicManager::GetPubkeyRsaCallback([this, address, messageToSign, isForcibly, fee, txHash, blockchainServ, blockchainName, makeFunc, processFunc](const QString &pubkeyRsa){
            emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, isForcibly, fee, pubkeyRsa, txHash, blockchainServ, blockchainName, makeFunc, processFunc](const QString &pubkey, const QString &sign){
                emit messenger->registerAddressFromBlockchain(isForcibly, address, pubkeyRsa, pubkey, sign, fee, txHash, blockchainServ, blockchainName, Messenger::RegisterAddressBlockchainCallback(processFunc, makeFunc.error, signalFunc));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::savePublicKeyCollocutor(bool isForcibly, QString address, QString collocutor) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgPublicKeyCollocutorGettedJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(collocutor));

    LOG << "savePublicKeyCollocutor " << address << " " << collocutor;

    wrapOperation([&, this](){
        const QString messageToSign = Messenger::makeTextForGetPubkeyRequest(collocutor);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, collocutor, isForcibly, makeFunc](const QString &pubkey, const QString &sign){
            emit messenger->savePubkeyAddress(isForcibly, collocutor, pubkey, sign, Messenger::SavePubkeyCallback([this, address, collocutor, makeFunc](bool /*isNew*/) {
                LOG << "Pubkey saved " << collocutor;
                makeFunc.func(TypedException(), address, collocutor);
            }, [this, address, collocutor, makeFunc](const TypedException &exception) {
                if (exception.numError != TypeErrors::MESSENGER_SERVER_ERROR_ADDRESS_NOT_FOUND) {
                    makeFunc.error(exception);
                    return;
                }
                const QString messageToSign = Messenger::makeTextForWantToTalkRequest(collocutor);
                emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, collocutor, makeFunc](const QString &pubkey, const QString &sign){
                    emit messenger->wantToTalk(collocutor, pubkey, sign, Messenger::WantToTalkCallback([this, address, collocutor, makeFunc](){
                        LOG << "Want to talk " << address << " " << collocutor;
                        makeFunc.func(TypedException(TypeErrors::MESSENGER_SERVER_ERROR_ADDRESS_NOT_FOUND, "Not found"), address, collocutor);
                    }, makeFunc.error, signalFunc));
                }, makeFunc.error, signalFunc));
            }, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getUserInfo(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "getUserInfoResultJs";

    LOG << "getUserInfo " << " " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit messenger->getUserInfo(address, Messenger::UserInfoCallback([address, makeFunc](bool complete, const ContactInfo &info) {
            makeFunc.func(TypedException(), address, contactInfoToJson(complete, info));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getCollocutorInfo(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "getCollocutorInfoResultJs";

    LOG << "getContactInfo " << " " << address;

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    wrapOperation([&, this](){
        emit messenger->getCollocutorInfo(address, Messenger::UserInfoCallback([address, makeFunc](bool isComplete, const ContactInfo &info) {
            makeFunc.func(TypedException(), address, contactInfoToJson(isComplete, info));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::sendMessage(QString address, QString collocutor, QString dataHex, QString timestampStr, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgMessageSendedJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(collocutor));

    LOG << "sendMessage " << " " << address << " " << collocutor << " " << timestampStr << " " << feeStr;

    wrapOperation([&, this](){
        CHECK(!dataHex.isEmpty(), "Empty message");
        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "fee field incorrect");
        const uint64_t timestamp = timestampStr.toULongLong(&isValid);
        CHECK(isValid, "timestamp field incorrect");
        emit messenger->getPubkeyAddress(collocutor, Messenger::GetPubkeyAddressCallback([this, makeFunc, address, collocutor, dataHex, fee, timestamp](const QString &pubkey) mutable {
            emit cryptoManager.encryptDataRsa(dataHex, pubkey, CryptographicManager::EncryptMessageCallback([this, address, collocutor, makeFunc, fee, timestamp, dataHex](const QString &encryptedDataToWss) {
                emit cryptoManager.encryptDataPrivateKey(dataHex, address, CryptographicManager::EncryptMessageCallback([this, address, collocutor, makeFunc, fee, timestamp, dataHex, encryptedDataToWss](const QString &encryptedDataToBd) {
                    const QString messageToSign = Messenger::makeTextForSendMessageRequest(collocutor, encryptedDataToWss, encryptedDataToBd, fee, timestamp);
                    emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, collocutor, makeFunc, fee, timestamp, dataHex, encryptedDataToWss, encryptedDataToBd](const QString &pubkey, const QString &sign) {
                        emit messenger->sendMessage(address, collocutor, false, "", encryptedDataToWss, dataHex, pubkey, sign, fee, timestamp, encryptedDataToBd, Messenger::SendMessageCallback([this, makeFunc, address, collocutor]() {
                            LOG << "Message sended " << address << " " << collocutor;
                            makeFunc.func(TypedException(), address, collocutor);
                        }, makeFunc.error, signalFunc));
                    }, makeFunc.error, signalFunc));
                }, makeFunc.error, signalFunc));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getLastMessageNumber(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgLastMessegesJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<Message::Counter>(0));

    LOG << "getLastMessageNumber " << address;

    wrapOperation([&, this](){
        emit messenger->getLastMessage(address, false, "", Messenger::GetSavedPosCallback([this, makeFunc, address](const Message::Counter &pos) {
            LOG << "getLastMessageNumber ok " << address << " " << pos;
            makeFunc.func(TypedException(), address, pos);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedPos(QString address, const QString &collocutor) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedPosJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(collocutor), JsTypeReturn<Message::Counter>(0));

    LOG << "getSavedPos " << address << " " << collocutor;

    wrapOperation([&, this](){
        emit messenger->getSavedPos(address, false, collocutor, Messenger::GetSavedPosCallback([this, makeFunc, address, collocutor](const Message::Counter &pos) {
            LOG << "getSavedPos ok " << address << " " << collocutor << " " << pos;
            makeFunc.func(TypedException(), address, collocutor, pos);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedsPos(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedsPosJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    LOG << "getSavedsPos " << address;

    wrapOperation([&, this](){
        emit messenger->getSavedsPos(address, false, Messenger::GetSavedsPosCallback([makeFunc, address](const std::vector<std::pair<QString, Message::Counter>> &pos) {
            const QJsonDocument result(allPosToJson(pos));

            LOG << "getSavedsPos ok " << address << " " << pos.size();
            makeFunc.func(TypedException(), address, result);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::savePos(QString address, const QString &collocutor, QString counterStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgStorePosJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(collocutor), JsTypeReturn<QString>("Not ok"));

    LOG << "savePos " << address << " " << collocutor << " " << counterStr;

    wrapOperation([&, this](){
        bool isValid;
        const Message::Counter counter = counterStr.toLongLong(&isValid);
        CHECK(isValid, "counter field invalid");
        emit messenger->savePos(address, false, collocutor, counter, Messenger::SavePosCallback([this, makeFunc, address, collocutor](){
            LOG << "savePos ok " << address << " " << collocutor;
            makeFunc.func(TypedException(), address, collocutor, "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getCountMessages(QString address, const QString &collocutor, QString from) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgCountMessagesJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(collocutor), JsTypeReturn<Message::Counter>(0));

    LOG << "getCountMessages " << address << " " << collocutor << " " << from;

    wrapOperation([&, this](){
        bool isValid;
        const Message::Counter fromI = from.toLongLong(&isValid);
        CHECK(isValid, "from field invalid");
        emit messenger->getCountMessages(address, collocutor, fromI, Messenger::GetCountMessagesCallback([this, makeFunc, address, collocutor](const Message::Counter &count) {
            LOG << "getCountMessages ok " << address << " " << collocutor << " " << count;
            makeFunc.func(TypedException(), address, collocutor, count);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
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

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(titleSha), JsTypeReturn<QString>(writer));

    LOG << "add writer " << address << " " << titleSha << " " << writer;

    wrapOperation([&, this](){
        const QString messageToSign = Messenger::makeTextForChannelAddWriterRequest(titleSha, writer);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, writer, titleSha, makeFunc](const QString &pubkey, const QString &sign) {
            emit messenger->addWriterToChannel(titleSha, writer, pubkey, sign, Messenger::AddWriterToChannelCallback([this, makeFunc, address, titleSha, writer]() {
                LOG << "add writer ok " << address << " " << titleSha << " " << writer;
                makeFunc.func(TypedException(), address, titleSha, writer);
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::delWriterFromChannel(QString address, QString titleSha, QString writer) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgDelWriterFromChannelJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(titleSha), JsTypeReturn<QString>(writer));

    LOG << "del writer " << address << " " << titleSha << " " << writer;

    wrapOperation([&, this](){
        const QString messageToSign = Messenger::makeTextForChannelDelWriterRequest(titleSha, writer);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, writer, titleSha, makeFunc](const QString &pubkey, const QString &sign) {
            emit messenger->delWriterFromChannel(titleSha, writer, pubkey, sign, Messenger::DelWriterToChannelCallback([this, makeFunc, address, titleSha, writer]() {
                LOG << "del writer ok " << address << " " << titleSha << " " << writer;
                makeFunc.func(TypedException(), address, titleSha, writer);
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::sendMessageToChannel(QString address, QString titleSha, QString dataHex, QString timestampStr, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgMessageSendedToChannelJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(titleSha));

    LOG << "sendMessageToChannel " << " " << address << " " << titleSha << " " << timestampStr << " " << feeStr;

    wrapOperation([&, this](){
        CHECK(!dataHex.isEmpty(), "Empty message");
        bool isValid;
        const uint64_t fee = feeStr.toULongLong(&isValid);
        CHECK(isValid, "Fee field invalid");
        const uint64_t timestamp = timestampStr.toULongLong(&isValid);
        CHECK(isValid, "timestamp field invalid");

        const QString messageToSign = Messenger::makeTextForSendToChannelRequest(titleSha, dataHex, fee, timestamp);
        emit cryptoManager.signMessage(address, messageToSign, CryptographicManager::SignMessageCallback([this, address, dataHex, titleSha, fee, timestamp, makeFunc](const QString &pubkey, const QString &sign) {
            emit messenger->sendMessage(address, address, true, titleSha, dataHex, dataHex, pubkey, sign, fee, timestamp, dataHex, Messenger::SendMessageCallback([this, makeFunc, address, titleSha]() {
                LOG << "sendMessageToChannel ok " << address << " " << titleSha;
                makeFunc.func(TypedException(), address, titleSha);
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getChannelsList(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetChannelListJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    LOG << "getChannelList " << " " << address;

    wrapOperation([&, this](){
        emit messenger->getChannelList(address, Messenger::GetChannelListCallback([this, makeFunc, address](const std::vector<ChannelInfo> &channels) {
            LOG << "getChannelList ok " << address << " " << channels.size();
            const QJsonDocument channelsJson = channelListToJson(channels);
            makeFunc.func(TypedException(), address, channelsJson);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getLastMessageChannelNumber(QString address, QString titleSha) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetLastMessageChannelJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(titleSha), JsTypeReturn<Message::Counter>(0));

    LOG << "get last message channel " << " " << address << " " << titleSha;

    wrapOperation([&, this](){
        emit messenger->getLastMessage(address, true, titleSha, Messenger::GetSavedPosCallback([this, makeFunc, address, titleSha](const Message::Counter &pos) {
            LOG << "get last message channel ok " << address << " " << pos;
            makeFunc.func(TypedException(), address, titleSha, pos);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedPosChannel(QString address, QString titleSha) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedPosChannelJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(titleSha), JsTypeReturn<Message::Counter>(0));

    LOG << "getSavedPosChannel " << address << " " << titleSha;

    wrapOperation([&, this](){
        emit messenger->getSavedPos(address, true, titleSha, Messenger::GetSavedPosCallback([this, makeFunc, address, titleSha](const Message::Counter &pos) {
            LOG << "getSavedPosChannel ok " << address << " " << titleSha << " " << pos;
            makeFunc.func(TypedException(), address, titleSha, pos);
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::savePosToChannel(QString address, const QString &titleSha, QString counterStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgStorePosToChannelJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(titleSha), JsTypeReturn<QString>("Not ok"));

    LOG << "savePosToChannel " << address << " " << titleSha << " " << counterStr;

    wrapOperation([&, this](){
        bool isValid;
        const Message::Counter counter = counterStr.toLongLong(&isValid);
        CHECK(isValid, "counter field invalid");
        emit messenger->savePos(address, true, titleSha, counter, Messenger::SavePosCallback([this, makeFunc, address, titleSha](){
            LOG << "savePosToChannel ok " << address << " " << titleSha;
            makeFunc.func(TypedException(), address, titleSha, "Ok");
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressChannel(QString address, QString titleSha, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressChannelJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(titleSha), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    LOG << "get messages channel " << address << " " << titleSha << " " << from << " " << to;

    wrapOperation([&, this](){
        bool isValid;
        const Message::Counter fromC = from.toLongLong(&isValid);
        CHECK(isValid, "from field invalid");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field invalid");
        emit messenger->getHistoryAddressAddress(address, true, titleSha, fromC, toC, Messenger::GetMessagesCallback([this, makeFunc, address, titleSha](const std::vector<Message> &messages) {
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, titleSha, makeFunc](const std::vector<Message> &messages){
                LOG << "get messages channel ok " << address << " " << titleSha << " " << messages.size();
                makeFunc.func(TypedException(), address, titleSha, messagesToJson(messages));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressChannelCount(QString address, QString titleSha, QString count, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressChannelCountJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address), JsTypeReturn<QString>(titleSha), JsTypeReturn<QJsonDocument>(QJsonDocument()));

    LOG << "get messagesCC " << address << " " << titleSha << " " << count << " " << to;

    wrapOperation([&, this](){
        bool isValid;
        const Message::Counter countC = count.toLongLong(&isValid);
        CHECK(isValid, "count field invalid");
        const Message::Counter toC = to.toLongLong(&isValid);
        CHECK(isValid, "to field invalid");

        emit messenger->getHistoryAddressAddressCount(address, true, titleSha, countC, toC, Messenger::GetMessagesCallback([this, makeFunc, address, titleSha](const std::vector<Message> &messages) {
            emit cryptoManager.decryptMessages(messages, address, CryptographicManager::DecryptMessagesCallback([address, titleSha, makeFunc](const std::vector<Message> &messages){
                LOG << "get messagesCC ok " << address << " " << titleSha << " " << messages.size();
                makeFunc.func(TypedException(), address, titleSha, messagesToJson(messages));
            }, makeFunc.error, signalFunc));
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::onNewMesseges(QString address, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgNewMessegesJs";
    LOG << "New messages " << address << " " << lastMessage;
    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, lastMessage);
END_SLOT_WRAPPER
}

void MessengerJavascript::onAddedToChannel(QString address, QString title, QString titleSha, QString admin, Message::Counter counter) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgAddedToChannelJs";
    LOG << "added to channel " << address << " " << titleSha << " " << counter;
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
    LOG << "New messages channel " << address << " " << titleSha << " " << lastMessage;
    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), address, titleSha, lastMessage);
END_SLOT_WRAPPER
}

void MessengerJavascript::setPathsImpl() {
    CHECK(messenger != nullptr, "Messenger not set");

    const QString walletPathFull = makePath(walletPath, mthWalletType);
    CHECK(!walletPathFull.isNull() && !walletPathFull.isEmpty(), "Incorrect path to wallet: empty");

    LOG << "Set messenger javascript path " << walletPathFull;

    const std::vector<std::pair<QString, QString>> vectors = Wallet::getAllWalletsInFolder(walletPathFull);
    std::vector<QString> addresses;
    addresses.reserve(vectors.size());
    std::transform(vectors.begin(), vectors.end(), std::back_inserter(addresses), [](const auto &pair) {
        return pair.first;
    });

    emit messenger->addAllAddressesInFolder(walletPathFull, addresses, Messenger::AddAllWalletsInFolderCallback([](){
        LOG << "addresses added";
    }, [](const TypedException &exception) {
        LOG << "addresses added exception " << exception.numError << " " << exception.description;
    }, signalFunc));
}

void MessengerJavascript::setMhcType(bool isMhc) {
BEGIN_SLOT_WRAPPER
    LOG << "Set mhc type: " << (isMhc ? "Mhc" : "Tmh");
    const QString JS_NAME_RESULT = "msgSetWalletTypeResultJs";
    const QString newMthWalletType = isMhc ? Wallet::WALLET_PATH_MTH : Wallet::WALLET_PATH_TMH;
    const TypedException &exception = apiVrapper2([&, this]{
        if (newMthWalletType != mthWalletType) {
            mthWalletType = newMthWalletType;
            setPathsImpl();
        }
    });
    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, isMhc);
END_SLOT_WRAPPER
}

void MessengerJavascript::unlockWallet(QString address, QString password, QString passwordRsa, int timeSeconds) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgUnlockWalletResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(address));

    const QString walletPathFull = makePath(walletPath, mthWalletType);
    LOG << "Unlock wallet " << address << " Wallet path " << walletPathFull << " timeout " << timeSeconds;
    wrapOperation([&, this](){
        CHECK_TYPED(!walletPathFull.isEmpty(), TypeErrors::MESSENGER_NOT_CONFIGURED, "Wallet path not set");

        emit cryptoManager.unlockWallet(walletPathFull, address, password, passwordRsa, seconds(timeSeconds), CryptographicManager::UnlockWalletCallback([this, address, makeFunc]() {
            emit messenger->decryptMessages(address, Messenger::DecryptUserMessagesCallback([this, address, makeFunc]() {
                LOG << "Unlock wallet ok " << address;
                makeFunc.func(TypedException(), address);
            }, makeFunc.error, signalFunc));
         }, makeFunc.error, signalFunc));

        emit messenger->isCompleteUser(address, Messenger::CompleteUserCallback([this, address, makeFunc](bool isComplete) {
            if (!isComplete) {
                const auto mFunc = [this, address, makeFunc](const TypedException &exception, const QString &/*result*/) {
                    makeFunc.func(exception, address);
                }; // TODO ???
                doRegisterAddress(address, false, true, mFunc, makeFunc.error);
            }
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::lockWallet() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgLockWalletResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT);

    LOG << "lock wallets";

    wrapOperation([&, this](){
        emit cryptoManager.lockWallet(CryptographicManager::LockWalletCallback([this, makeFunc]() {
            makeFunc.func(TypedException());
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::remainingTime() {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgRemainingTimeResultJs";

    const auto makeFunc = makeJavascriptReturnAndErrorFuncs(JS_NAME_RESULT, JsTypeReturn<QString>(""), JsTypeReturn<size_t>(0));

    LOG << "Remaining time";
    wrapOperation([&, this](){
        emit cryptoManager.remainingTime(CryptographicManager::RemainingTimeCallback([this, makeFunc](const QString &address, const seconds &elapsed) {
            makeFunc.func(TypedException(), address, elapsed.count());
        }, makeFunc.error, signalFunc));
    }, makeFunc.error);
END_SLOT_WRAPPER
}

void MessengerJavascript::reEmit() {
BEGIN_SLOT_WRAPPER
    LOG << "Messenger Reemit";
    CHECK(messenger != nullptr, "Messenger not set");
    emit messenger->reEmit();
END_SLOT_WRAPPER
}

void MessengerJavascript::onLogined(bool isInit, const QString login) {
BEGIN_SLOT_WRAPPER
    QString newWalletPath;
    if (!login.isEmpty()) {
        newWalletPath = makePath(getWalletPath(), login);
    } else {
        newWalletPath = makePath(getWalletPath(), defaultUserName);
    }
    if (newWalletPath != walletPath) {
        walletPath = newWalletPath;
        setPathsImpl();
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::onMthWalletCreated(const QString &wallet)
{
    Q_UNUSED(wallet);
BEGIN_SLOT_WRAPPER
    setPathsImpl();
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
