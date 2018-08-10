#include "MessengerJavascript.h"

#include "check.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "SlotWrapper.h"

#include "Wallet.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

/*static Wallet getWalletInst() {
    // Узнать каталог к ключу
    std::string pubkey;
    std::string addr;
    Wallet::createWallet("./", "123", pubkey, addr);
    Wallet wallet("./", addr, password);
}*/

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

static QJsonDocument messagesToJson(const std::vector<Message> &messages) {
    QJsonArray messagesArrJson;
    for (const Message &message: messages) {
        QJsonObject messageJson;
        // Расшифровать data
        messageJson.insert("collocutor", message.collocutor);
        messageJson.insert("isInput", message.isInput);
        messageJson.insert("timestamp", QString::fromStdString(std::to_string(message.timestamp)));
        messageJson.insert("data", message.data);
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
        emit messenger->getHistoryAddress(address, fromC, toC, Messenger::GetMessagesCallback([this, JS_NAME_RESULT, address](const std::vector<Message> &messages) {
            // TODO добавить обработку ошибок
            const Opt<QJsonDocument> result(messagesToJson(messages));
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), result);
        }));
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
        emit messenger->getHistoryAddressAddress(address, collocutor, fromC, toC, Messenger::GetMessagesCallback([this, JS_NAME_RESULT, address, collocutor](const std::vector<Message> &messages) {
            // TODO добавить обработку ошибок
            const Opt<QJsonDocument> result(messagesToJson(messages));
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(collocutor), result);
        }));
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
        emit messenger->getHistoryAddressAddressCount(address, collocutor, countC, toC, Messenger::GetMessagesCallback([this, JS_NAME_RESULT, address, collocutor](const std::vector<Message> &messages) {
            // TODO добавить обработку ошибок
            const Opt<QJsonDocument> result(messagesToJson(messages));
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(collocutor), result);
        }));
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
        emit messenger->registerAddress(isForcibly, address, "", "", "", fee, [this, JS_NAME_RESULT, address](bool isNew) {
            if (isNew) {
                const std::vector<QString> messagesForSign = Messenger::stringsForSign();
                // Подписать сообщения
                const std::vector<QString> result;
                emit messenger->signedStrings(result, [this, JS_NAME_RESULT, address](){
                    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>("Ok"));
                });
            }
        });
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>("Not ok"));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::getPublicKeyCollocutor(bool isForcibly, QString address, QString collocutor) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgPublicKeyCollocutorGettedJs";

    LOG << "get messages";

    const TypedException exception = apiVrapper([&, this](){
        emit messenger->getPubkeyAddress(isForcibly, collocutor, "", "", [this, JS_NAME_RESULT, address, collocutor](bool isNew) {
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
        // Подписать данные
        const QString encryptedDataToWss = "";
        const QString encryptedDataToBd = "";
        emit messenger->sendMessage(address, collocutor, encryptedDataToWss, "", "", fee, timestamp, encryptedDataToBd, [this, JS_NAME_RESULT, address, collocutor]() {
            makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(collocutor));
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
