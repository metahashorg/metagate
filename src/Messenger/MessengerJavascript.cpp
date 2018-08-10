#include "MessengerJavascript.h"

#include "check.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "SlotWrapper.h"
#include "utils.h"

#include "Wallet.h"
#include "WalletRsa.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

static std::pair<Wallet, WalletRsa> getWalletInst2() {
    // Узнать каталог к ключу
    std::string pubkey;
    std::string addr;
    Wallet::createWallet("./", "123", pubkey, addr);
    Wallet wallet("./", addr, "123");

    WalletRsa::createRsaKey("./", addr, "123");
    WalletRsa walletRsa("./", addr);
    walletRsa.unlock("123");

    return std::make_pair(wallet, std::move(walletRsa));
}

static std::pair<Wallet, WalletRsa>& getWalletInst() {
    static auto pair = getWalletInst2();

    return pair;
}

MessengerJavascript::MessengerJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &MessengerJavascript::callbackCall, this, &MessengerJavascript::onCallbackCall), "not connect onCallbackCall");

    CHECK(connect(this, &MessengerJavascript::operationUnluckySig, this, &MessengerJavascript::onOperationUnlucky), "not connect onOperationUnlucky");
    CHECK(connect(this, &MessengerJavascript::newMessegesSig, this, &MessengerJavascript::onNewMesseges), "not connect onNewMesseges");
}

void MessengerJavascript::onCallbackCall(const std::function<void()> &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

template<class Function>
TypedException MessengerJavascript::apiVrapper(const Function &func) {
    try {
        func();
        return TypedException();
    } catch (const TypedException &e) {
        LOG << "Error " << std::to_string(e.numError) << ". " << e.description;
        return e;
    } catch (const Exception &e) {
        LOG << "Error " << e;
        return TypedException(TypeErrors::OTHER_ERROR, e);
    } catch (const std::exception &e) {
        LOG << "Error " << e.what();
        return TypedException(TypeErrors::OTHER_ERROR, e.what());
    } catch (...) {
        LOG << "Unknown error";
        return TypedException(TypeErrors::OTHER_ERROR, "Unknown error");
    }
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
        const std::string decryptedData = toHex(walletRsa.decryptMessage(message.data.toStdString()));

        messageJson.insert("collocutor", message.collocutor);
        messageJson.insert("isInput", message.isInput);
        messageJson.insert("timestamp", QString::fromStdString(std::to_string(message.timestamp)));
        messageJson.insert("data", QString::fromStdString(decryptedData));
        messageJson.insert("counter", QString::fromStdString(std::to_string(message.counter)));
        messageJson.insert("fee", QString::fromStdString(std::to_string(message.fee)));
        messagesArrJson.push_back(messageJson);
    }
    return QJsonDocument(messagesArrJson);
}

void MessengerJavascript::getHistoryAddress(QString address, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressJs";

    LOG << "get messages";

    const Message::Counter fromC = std::stoull(from.toStdString());
    const Message::Counter toC = std::stoull(to.toStdString());
    Opt<QJsonDocument> result;
    const TypedException exception = apiVrapper([&, this](){
        emit messenger->getHistoryAddress(address, fromC, toC, [this, JS_NAME_RESULT, address](const std::vector<Message> &messages) {
            Opt<QJsonDocument> result;
            const TypedException exception = apiVrapper([&, this](){
                const auto &pairWallet = getWalletInst();
                result = messagesToJson(messages, pairWallet.second);
            });
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), result);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddress(QString address, QString collocutor, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressJs";

    LOG << "get messages";

    const Message::Counter fromC = std::stoull(from.toStdString());
    const Message::Counter toC = std::stoull(to.toStdString());
    Opt<QJsonDocument> result;
    const TypedException exception = apiVrapper([&, this](){
        emit messenger->getHistoryAddressAddress(address, collocutor, fromC, toC, [this, JS_NAME_RESULT, address, collocutor](const std::vector<Message> &messages) {
            Opt<QJsonDocument> result;
            const TypedException exception = apiVrapper([&, this](){
                const auto &pairWallet = getWalletInst();
                result = messagesToJson(messages, pairWallet.second);
            });
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), result);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddressCount(QString address, QString collocutor, QString count, QString to) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressCountJs";

    LOG << "get messages";

    const Message::Counter countC = std::stoull(count.toStdString());
    const Message::Counter toC = std::stoull(to.toStdString());
    Opt<QJsonDocument> result;
    const TypedException exception = apiVrapper([&, this](){
        emit messenger->getHistoryAddressAddressCount(address, collocutor, countC, toC, [this, JS_NAME_RESULT, address, collocutor](const std::vector<Message> &messages) {
            Opt<QJsonDocument> result;
            const TypedException exception = apiVrapper([&, this](){
                const auto &pairWallet = getWalletInst();
                result = messagesToJson(messages, pairWallet.second);
            });
            makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), result);
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor), result);
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::registerAddress(bool isForcibly, QString address, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgAddressAppendToMessengerJs";

    LOG << "get messages";

    const uint64_t fee = std::stoull(feeStr.toStdString());
    const TypedException exception = apiVrapper([&, this](){
        const auto &pairWallet = getWalletInst();
        const QString messageToSign = Messenger::makeTextForSignRegisterRequest(address, QString::fromStdString(pairWallet.second.getPublikKey()), fee);
        std::string pubkey;
        const std::string &sign = pairWallet.first.sign(messageToSign.toStdString(), pubkey);
        emit messenger->registerAddress(isForcibly, address, QString::fromStdString(pairWallet.second.getPublikKey()), QString::fromStdString(pubkey), QString::fromStdString(sign), fee, [this, JS_NAME_RESULT, address](bool isNew) {
            const TypedException exception = apiVrapper([&, this](){
                if (isNew) {
                    const std::vector<QString> messagesForSign = Messenger::stringsForSign();
                    const auto &pairWallet = getWalletInst();
                    std::vector<QString> result;
                    for (const QString &msg: messagesForSign) {
                        std::string tmp;
                        const std::string sign = pairWallet.first.sign(msg.toStdString(), tmp);
                        result.emplace_back(QString::fromStdString(sign));
                    }
                    emit messenger->signedStrings(result, [this, JS_NAME_RESULT, address](){
                        makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>("Ok"));
                    });
                }
            });
            if (exception.isSet()) {
                makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>("Not ok"));
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

    LOG << "get messages";

    const TypedException exception = apiVrapper([&, this](){
        const auto &pairWallet = getWalletInst();
        const QString messageToSign = Messenger::makeTextForGetPubkeyRequest(collocutor);
        std::string pubkey;
        const std::string &sign = pairWallet.first.sign(messageToSign.toStdString(), pubkey);

        emit messenger->savePubkeyAddress(isForcibly, collocutor, QString::fromStdString(pubkey), QString::fromStdString(sign), [this, JS_NAME_RESULT, address, collocutor](bool /*isNew*/) {
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(collocutor));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::sendMessage(QString address, QString collocutor, QString data, QString timestampStr, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgMessageSendedJs";

    LOG << "get messages";

    const TypedException exception = apiVrapper([&, this](){
        const uint64_t fee = std::stoull(feeStr.toStdString());
        const uint64_t timestamp = std::stoull(timestampStr.toStdString());
        emit messenger->getPubkeyAddress(collocutor, [this, JS_NAME_RESULT, address, collocutor, data, fee, timestamp](const QString &pubkey) {
            const TypedException exception = apiVrapper([&, this](){
                const WalletRsa walletRsa = WalletRsa::fromPublicKey(pubkey.toStdString());
                const QString encryptedDataToWss = QString::fromStdString(walletRsa.encrypt(data.toStdString()));

                const auto &pairWallet = getWalletInst();
                const QString messageToSign = Messenger::makeTextForSendMessageRequest(collocutor, encryptedDataToWss, fee);
                std::string pub;
                const std::string &sign = pairWallet.first.sign(messageToSign.toStdString(), pub);

                const QString encryptedDataToBd = QString::fromStdString(pairWallet.second.encrypt(data.toStdString()));
                emit messenger->sendMessage(address, collocutor, encryptedDataToWss, QString::fromStdString(pub), QString::fromStdString(sign), fee, timestamp, encryptedDataToBd, [this, JS_NAME_RESULT, address, collocutor]() {
                    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(collocutor));
                });
            });
            if (exception.isSet()) {
                makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>(collocutor));
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

    LOG << "get messages";

    const TypedException exception = apiVrapper([&, this](){
        emit messenger->getLastMessage(address, [this, JS_NAME_RESULT, address](const Message::Counter &pos) {
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<Message::Counter>(pos));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<Message::Counter>(0));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getSavedPos(QString address) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgSavedPosJs";

    LOG << "get messages";

    const TypedException exception = apiVrapper([&, this](){
        emit messenger->getSavedPos(address, [this, JS_NAME_RESULT, address](const Message::Counter &pos) {
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<Message::Counter>(pos));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<Message::Counter>(0));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::savePos(QString address, QString counterStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgStorePosJs";

    LOG << "get messages";

    const TypedException exception = apiVrapper([&, this](){
        const Message::Counter counter = std::stoull(counterStr.toStdString());
        emit messenger->savePos(address, counter, [this, JS_NAME_RESULT, address]{
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>("Ok"));
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<QString>("Not ok"));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::onOperationUnlucky(int operation, QString address, QString description) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgOperationUnluckyJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<int>(operation), Opt<QString>(description));
END_SLOT_WRAPPER
}

void MessengerJavascript::onNewMesseges(QString address, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgNewMessegesJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<Message::Counter>(lastMessage));
END_SLOT_WRAPPER
}

void MessengerJavascript::runJs(const QString &script) {
    emit jsRunSig(script);
}
