#ifndef MESSENGER_H
#define MESSENGER_H

#include <QObject>

#include "TimerClass.h"
#include "WebSocketClient.h"

#include "Message.h"

#include <map>

struct NewMessageResponse;
class MessengerJavascript;

class Messenger : public TimerClass
{
    Q_OBJECT
private:

    class DeferredMessage {
    public:

        bool check() const {
            if (!isDeferred_) {
                return false;
            }
            const time_point now = ::now();
            const milliseconds duration = std::chrono::duration_cast<milliseconds>(now - begin_);
            return duration >= elapse_;
        }

        bool isDeferred() const {
            return isDeferred_;
        }

        void setDeferred(const milliseconds &elapse) {
            isDeferred_ = true;
            begin_ = ::now();
            elapse_ = elapse;
        }

        void resetDeferred() {
            isDeferred_ = false;
        }

    private:
        bool isDeferred_ = false;
        time_point begin_;
        milliseconds elapse_;
    };

public:

    explicit Messenger(MessengerJavascript &javascriptWrapper, QObject *parent = nullptr);

public:

    static std::vector<QString> stringsForSign();

    static QString makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee);

    static QString makeTextForGetPubkeyRequest(const QString &address);

    static QString makeTextForSendMessageRequest(const QString &address, const QString &dataHex, uint64_t fee);

signals:

    void registerAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee);

    void getPubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex);

    void sendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex);

    void signedStrings(const std::vector<QString> &signedHexs);

    void getLastMessage(const QString &address);

    void getSavedPos(const QString &address);

    void savePos(const QString &address, Message::Counter pos);

    void getHistoryAddress(QString address, Message::Counter from, Message::Counter to);

    void getHistoryAddressAddress(QString address, QString collocutor, Message::Counter from, Message::Counter to);

    void getHistoryAddressAddressCount(QString address, QString collocutor, Message::Counter count, Message::Counter to);

private slots:

    void onRegisterAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee);

    void onGetPubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex);

    void onSendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex);

    void onSignedStrings(const std::vector<QString> &signedHexs);

    void onGetLastMessage(const QString &address);

    void onGetSavedPos(const QString &address);

    void onSavePos(const QString &address, Message::Counter pos);

    void onGetHistoryAddress(QString address, Message::Counter from, Message::Counter to);

    void onGetHistoryAddressAddress(QString address, QString collocutor, Message::Counter from, Message::Counter to);

    void onGetHistoryAddressAddressCount(QString address, QString collocutor, Message::Counter count, Message::Counter to);

private slots:

    void onRun();

    void onTimerEvent();

    void onWssMessageReceived(QString message);

private:

    void getMessagesFromAddressFromWss(const QString &fromAddress, Message::Counter from, Message::Counter to);

    void clearAddressesToMonitored();

    void addAddressToMonitored(const QString &address);

    void processMessages(const QString &address, const std::vector<NewMessageResponse> &messages);

    QString getSignFromMethod(const QString &address, const QString &method) const;

    std::vector<QString> getMonitoredAddresses() const;

private:

    MessengerJavascript &javascriptWrapper;

    WebSocketClient wssClient;

    std::map<QString, DeferredMessage> deferredMessages;

};

#endif // MESSENGER_H
