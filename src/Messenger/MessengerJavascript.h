#ifndef MESSENGERJAVASCRIPT_H
#define MESSENGERJAVASCRIPT_H

#include <QObject>

#include "TypedException.h"

#include <functional>

#include "Messenger.h"
#include "Message.h"

class MessengerJavascript : public QObject {
    Q_OBJECT
public:

    using Callback = std::function<void()>;

public:
    explicit MessengerJavascript(QObject *parent = nullptr);

    void setMessenger(Messenger &m) {
        messenger = &m;
    }

signals:

    void jsRunSig(QString jsString);

    void callbackCall(const Callback &callback);

public slots:

    void onCallbackCall(const Callback &callback);

signals:

    void operationUnluckySig(int operation, QString address, QString description);

    void newMessegesSig(QString address, Message::Counter lastMessage);

private slots:

    void onOperationUnlucky(int operation, QString address, QString description);

    void onNewMesseges(QString address, Message::Counter lastMessage);

public slots:

    Q_INVOKABLE void getHistoryAddress(QString address, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddress(QString address, QString collocutor, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddressCount(QString address, QString collocutor, QString count, QString to);

    Q_INVOKABLE void registerAddress(bool isForcibly, QString address, QString feeStr);

    Q_INVOKABLE void savePublicKeyCollocutor(bool isForcibly, QString address, QString collocutor);

    Q_INVOKABLE void sendMessage(QString address, QString collocutor, QString data, QString timestampStr, QString feeStr);

    Q_INVOKABLE void getLastMessageNumber(QString address);

    Q_INVOKABLE void getSavedPos(QString address);

    Q_INVOKABLE void savePos(QString address, QString counterStr);

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