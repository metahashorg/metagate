#ifndef MESSENGERJAVASCRIPT_H
#define MESSENGERJAVASCRIPT_H

#include <QObject>

#include <functional>

#include "Message.h"
#include "MessengerWaletManager.h"

struct TypedException;
class Messenger;

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

    void newMessegesSig(QString address, Message::Counter lastMessage);

    void addedToChannelSig(QString address, QString title, QString titleSha, QString admin, Message::Counter counter);

    void deletedFromChannelSig(QString address, QString title, QString titleSha, QString admin);

    void newMessegesChannelSig(QString address, QString titleSha, Message::Counter lastMessage);

private slots:

    void onNewMesseges(QString address, Message::Counter lastMessage);

    void onAddedToChannel(QString address, QString title, QString titleSha, QString admin, Message::Counter counter);

    void onDeletedFromChannel(QString address, QString title, QString titleSha, QString admin);

    void onNewMessegesChannel(QString address, QString titleSha, Message::Counter lastMessage);

public slots:

    Q_INVOKABLE void getHistoryAddress(QString address, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddress(QString address, QString collocutor, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressAddressCount(QString address, QString collocutor, QString count, QString to);

    Q_INVOKABLE void registerAddress(bool isForcibly, QString address, QString feeStr);

    Q_INVOKABLE void savePublicKeyCollocutor(bool isForcibly, QString address, QString collocutor);

    Q_INVOKABLE void sendMessage(QString address, QString collocutor, QString dataHex, QString timestampStr, QString feeStr);

    Q_INVOKABLE void getLastMessageNumber(QString address);

    Q_INVOKABLE void getSavedPos(QString address, const QString &collocutor);

    Q_INVOKABLE void getSavedsPos(QString address);

    Q_INVOKABLE void savePos(QString address, const QString &collocutor, QString counterStr);

    Q_INVOKABLE void getCountMessages(QString address, const QString &collocutor, QString from);


    Q_INVOKABLE void createChannel(QString address, QString channelTitle, QString fee);

    Q_INVOKABLE void addWriterToChannel(QString address, QString titleSha, QString writer);

    Q_INVOKABLE void delWriterFromChannel(QString address, QString titleSha, QString writer);

    Q_INVOKABLE void sendMessageToChannel(QString address, QString titleSha, QString dataHex, QString timestampStr, QString feeStr);

    Q_INVOKABLE void getChannelsList(QString address);

    Q_INVOKABLE void getLastMessageChannelNumber(QString address, QString titleSha);

    Q_INVOKABLE void getSavedPosChannel(QString address, QString titleSha);

    Q_INVOKABLE void savePosToChannel(QString address, const QString &titleSha, QString counterStr);

    Q_INVOKABLE void getHistoryAddressChannel(QString address, QString titleSha, QString from, QString to);

    Q_INVOKABLE void getHistoryAddressChannelCount(QString address, QString titleSha, QString count, QString to);


    Q_INVOKABLE void setPaths(QString newPatch, QString newUserName);

    Q_INVOKABLE void unlockWallet(QString address, QString password, QString passwordRsa, int timeSeconds);

    Q_INVOKABLE void lockWallet();

private:

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:

    Messenger *messenger = nullptr;

    MessengerWaletManager walletManager;

    QString walletPath;

};

#endif // MESSENGERJAVASCRIPT_H
