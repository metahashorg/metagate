#ifndef MESSENGERJAVASCRIPT_H
#define MESSENGERJAVASCRIPT_H

#include <QObject>

#include "TypedException.h"

#include "Messenger.h"
#include "Message.h"

class MessengerJavascript : public QObject {
    Q_OBJECT
public:
    explicit MessengerJavascript(QObject *parent = nullptr);

    void setMessenger(Messenger &m) {
        messenger = &m;
    }

signals:

    void jsRunSig(QString jsString);

signals:

    void operationUnluckySig(int operation, QString address, QString description);

    void addressAppendToMessengerSig(QString address);

    void messageSendedSig(QString address, QString collocutor);

    void publicKeyCollocutorGettedSig(QString address, QString collocutor);

    void newMessegesSig(QString address, Message::Counter lastMessage);

    void lastMessageSig(QString address, Message::Counter lastMessage);

    void savedPosSig(QString address, Message::Counter lastMessage);

    void storePosSig(QString address);

    void getHistoryAddressAddressSig(QString address, const std::vector<Message> &messages);

    void getHistoryAddressSig(QString address, const std::vector<Message> &messages);

    void getHistoryAddressAddressCountSig(QString address, const std::vector<Message> &messages);

private slots:

    void onMessageSended(QString address, QString collocutor);

    void onOperationUnlucky(int operation, QString address, QString description);

    void onAddressAppendToMessengerSig(QString address);

    void onPublicKeyCollocutorGettedSig(QString address, QString collocutor);

    void onNewMesseges(QString address, Message::Counter lastMessage);

    void onLastMessageSig(QString address, Message::Counter lastMessage);

    void onSavedPos(QString address, Message::Counter lastMessage);

    void onStorePos(QString address);

    void onGetHistoryAddressAddress(QString address, const std::vector<Message> &messages);

    void onGetHistoryAddress(QString address, const std::vector<Message> &messages);

    void onGetHistoryAddressAddressCount(QString address, const std::vector<Message> &messages);

public slots:

    Q_INVOKABLE void getHistoryAddress(QString address, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddress(QString address, QString collocutor, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddressCount(QString address, QString collocutor, QString count, QString to);

    Q_INVOKABLE void registerAddress(bool isForcibly, QString address, QString feeStr);

    Q_INVOKABLE void getPublicKeyCollocutor(bool isForcibly, QString address, QString collocutor);

    Q_INVOKABLE void sendMessage(QString address, QString collocutor, QString data, QString timestampStr, QString feeStr);

    Q_INVOKABLE void getLastMessageNumber(QString address);

    Q_INVOKABLE void getSavedPos(QString address);

    Q_INVOKABLE void savedPos(QString address, QString counterStr);

private:

    template<class Function>
    TypedException apiVrapper(const Function &func);

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:

    Messenger *messenger = nullptr;

};

#endif // MESSENGERJAVASCRIPT_H
