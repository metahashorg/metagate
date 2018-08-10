#include "MessengerJavascript.h"

#include "check.h"
#include "Log.h"
#include "makeJsFunc.h"
#include "SlotWrapper.h"

#include "Wallet.h"

#include <QJsonArray>
#include <QJsonValue>
#include <QJsonObject>

MessengerJavascript::MessengerJavascript(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(this, &MessengerJavascript::messageSendedSig, this, &MessengerJavascript::onMessageSended), "not connect onMessageSended");
    CHECK(connect(this, &MessengerJavascript::publicKeyCollocutorGettedSig, this, &MessengerJavascript::onPublicKeyCollocutorGettedSig), "not connect onPublicKeyCollocutorGettedSig");
    CHECK(connect(this, &MessengerJavascript::addressAppendToMessengerSig, this, &MessengerJavascript::onAddressAppendToMessengerSig), "not connect onAddressAppendToMessengerSig");
    CHECK(connect(this, &MessengerJavascript::operationUnluckySig, this, &MessengerJavascript::onOperationUnlucky), "not connect onOperationUnlucky");
    CHECK(connect(this, &MessengerJavascript::newMessegesSig, this, &MessengerJavascript::onNewMesseges), "not connect onNewMesseges");
    CHECK(connect(this, &MessengerJavascript::lastMessageSig, this, &MessengerJavascript::onLastMessageSig), "not connect onLastMessageSig");
    CHECK(connect(this, &MessengerJavascript::savedPosSig, this, &MessengerJavascript::onSavedPos), "not connect onSavedPos");
    CHECK(connect(this, &MessengerJavascript::storePosSig, this, &MessengerJavascript::onStorePos), "not connect onStorePos");
}

template<class Function>
TypedException MessengerJavascript::apiVrapper(const Function &func) {
    // TODO когда будет if constexpr, объединить обе функции в одну
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

void MessengerJavascript::getHistoryAddress(QString requestId, QString address, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgGetHistoryAddressJs";

    LOG << "get messages";

    const Message::Counter fromC = std::stoull(from.toStdString());
    const Message::Counter toC = std::stoull(to.toStdString());
    Opt<QJsonDocument> result;
    const TypedException exception = apiVrapper([&, this](){
        QJsonArray messagesArrJson;
        QJsonObject messageJson;
        messageJson.insert("collocutor", "");
        messageJson.insert("isInput", true);
        messageJson.insert("timestamp", 100);
        messageJson.insert("data", "data");
        messageJson.insert("counter", 1000);
        messageJson.insert("fee", 1000);
        messagesArrJson.push_back(messageJson);
        result = QJsonDocument(messagesArrJson);
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddress(QString requestId, QString address, QString collocutor, QString from, QString to) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressJs";

    LOG << "get messages";

    const Message::Counter fromC = std::stoull(from.toStdString());
    const Message::Counter toC = std::stoull(to.toStdString());
    Opt<QJsonDocument> result;
    const TypedException exception = apiVrapper([&, this](){
        QJsonArray messagesArrJson;
        result = QJsonDocument(messagesArrJson);
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void MessengerJavascript::getHistoryAddressAddressCount(QString requestId, QString address, QString collocutor, QString count, QString to) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgGetHistoryAddressAddressCountJs";

    LOG << "get messages";

    const Message::Counter countC = std::stoull(count.toStdString());
    const Message::Counter toC = std::stoull(to.toStdString());
    Opt<QJsonDocument> result;
    const TypedException exception = apiVrapper([&, this](){
        QJsonArray messagesArrJson;
        result = QJsonDocument(messagesArrJson);
    });

    makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(requestId), result);
END_SLOT_WRAPPER
}

void MessengerJavascript::registerAddress(bool isForcibly, QString address, QString feeStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgAddressAppendToMessengerJs";

    LOG << "get messages";

    const uint64_t fee = std::stoull(feeStr.toStdString());
    const TypedException exception = apiVrapper([&, this](){
        emit messenger->registerAddress(isForcibly, address, "", "", "", fee);
        emit messenger->signedStrings({});
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
        emit messenger->getPubkeyAddress(isForcibly, collocutor, "", "");
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
        emit messenger->sendMessage(address, collocutor, encryptedDataToWss, "", "", fee, timestamp, encryptedDataToBd);
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
        emit messenger->getLastMessage(address);
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
        emit messenger->getSavedPos(address);
    });

    if (exception.isSet()) {
        makeAndRunJsFuncParams(JS_NAME_RESULT, exception, Opt<QString>(address), Opt<Message::Counter>(0));
    }
END_SLOT_WRAPPER
}

void MessengerJavascript::savedPos(QString address, QString counterStr) {
BEGIN_SLOT_WRAPPER
    CHECK(messenger != nullptr, "Messenger not set");

    const QString JS_NAME_RESULT = "msgStorePosJs";

    LOG << "get messages";

    const TypedException exception = apiVrapper([&, this](){
        const Message::Counter counter = std::stoull(counterStr.toStdString());
        emit messenger->savePos(address, counter);
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

void MessengerJavascript::onAddressAppendToMessengerSig(QString address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgAddressAppendToMessengerJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(QString("Ok")));
END_SLOT_WRAPPER}

void MessengerJavascript::onMessageSended(QString address, QString collocutor) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgMessageSendedJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(collocutor));
END_SLOT_WRAPPER
}

void MessengerJavascript::onPublicKeyCollocutorGettedSig(QString address, QString collocutor) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgPublicKeyCollocutorGettedJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>(collocutor));
END_SLOT_WRAPPER
}

void MessengerJavascript::onNewMesseges(QString address, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgNewMessegesJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<Message::Counter>(lastMessage));
END_SLOT_WRAPPER
}

void MessengerJavascript::onLastMessageSig(QString address, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgLastMessegesJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<Message::Counter>(lastMessage));
END_SLOT_WRAPPER
}

void MessengerJavascript::onSavedPos(QString address, Message::Counter lastMessage) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgSavedPosJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<Message::Counter>(lastMessage));
END_SLOT_WRAPPER
}

void MessengerJavascript::onStorePos(QString address) {
BEGIN_SLOT_WRAPPER
    const QString JS_NAME_RESULT = "msgStorePosJs";

    makeAndRunJsFuncParams(JS_NAME_RESULT, TypedException(), Opt<QString>(address), Opt<QString>("Ok"));
END_SLOT_WRAPPER
}

void MessengerJavascript::runJs(const QString &script) {
    emit jsRunSig(script);
}
