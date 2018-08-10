#ifndef MESSENGERJAVASCRIPT_H
#define MESSENGERJAVASCRIPT_H

#include <QObject>

#include "TypedException.h"
#include "Messenger.h"

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

    void newMessegesSig(QString address, Messenger::Counter lastMessage);

    void lastMessageSig(QString address, Messenger::Counter lastMessage);

    void savedPosSig(QString address, Messenger::Counter lastMessage);

    void storePosSig(QString address);

private slots:

    void onMessageSended(QString address, QString collocutor);

    void onOperationUnlucky(int operation, QString address, QString description);

    void onAddressAppendToMessengerSig(QString address);

    void onPublicKeyCollocutorGettedSig(QString address, QString collocutor);

    void onNewMesseges(QString address, Messenger::Counter lastMessage);

    void onLastMessageSig(QString address, Messenger::Counter lastMessage);

    void onSavedPos(QString address, Messenger::Counter lastMessage);

    void onStorePos(QString address);

public slots:

    Q_INVOKABLE void getHistoryAddress(QString requestId, QString address, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddress(QString requestId, QString address, QString collocutor, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddressCount(QString requestId, QString address, QString collocutor, QString count, QString to);

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
