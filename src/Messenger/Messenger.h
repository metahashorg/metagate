#ifndef MESSENGER_H
#define MESSENGER_H

#include <QObject>

#include "TimerClass.h"
#include "WebSocketClient.h"

#include "RequestId.h"

#include "Message.h"

#include <map>
#include <unordered_map>
#include <functional>

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

    using GetMessagesCallback = std::function<void(const std::vector<Message> &messages)>;

    using SavePosCallback = std::function<void()>;

    using GetSavedPosCallback = std::function<void(const Message::Counter &pos)>;

    using RegisterAddressCallback = std::function<void(bool isNew)>;

    using SignedStringsCallback = std::function<void()>;

    using SavePubkeyCallback = std::function<void(bool isNew)>;

    using GetPubkeyAddress = std::function<void(const QString &pubkey)>;

    using SendMessageCallback = std::function<void()>;

public:

    explicit Messenger(MessengerJavascript &javascriptWrapper, QObject *parent = nullptr);

public:

    static std::vector<QString> stringsForSign();

    static QString makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee);

    static QString makeTextForGetPubkeyRequest(const QString &address);

    static QString makeTextForSendMessageRequest(const QString &address, const QString &dataHex, uint64_t fee);

signals:

    void registerAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const RegisterAddressCallback &callback);

    void savePubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex, const SavePubkeyCallback &callback);

    void getPubkeyAddress(const QString &address, const GetPubkeyAddress &callback);

    void sendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback);

    void signedStrings(const std::vector<QString> &signedHexs, const SignedStringsCallback &callback);

    void getLastMessage(const QString &address, const GetSavedPosCallback &callback);

    void getSavedPos(const QString &address, const GetSavedPosCallback &callback);

    void savePos(const QString &address, Message::Counter pos, const SavePosCallback &callback);

    void getHistoryAddress(QString address, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void getHistoryAddressAddress(QString address, QString collocutor, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void getHistoryAddressAddressCount(QString address, QString collocutor, Message::Counter count, Message::Counter to, const GetMessagesCallback &callback);

private slots:

    void onRegisterAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const RegisterAddressCallback &callback);

    void onSavePubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex, const SavePubkeyCallback &callback);

    void onGetPubkeyAddress(const QString &address, const GetPubkeyAddress &callback);

    void onSendMessage(const QString &thisAddress, const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback);

    void onSignedStrings(const std::vector<QString> &signedHexs, const SignedStringsCallback &callback);

    void onGetLastMessage(const QString &address, const GetSavedPosCallback &callback);

    void onGetSavedPos(const QString &address, const GetSavedPosCallback &callback);

    void onSavePos(const QString &address, Message::Counter pos, const SavePosCallback &callback);

    void onGetHistoryAddress(QString address, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void onGetHistoryAddressAddress(QString address, QString collocutor, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void onGetHistoryAddressAddressCount(QString address, QString collocutor, Message::Counter count, Message::Counter to, const GetMessagesCallback &callback);

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

    void invokeCallback(size_t requestId);

private:

    MessengerJavascript &javascriptWrapper;

    WebSocketClient wssClient;

    std::map<QString, DeferredMessage> deferredMessages;

    RequestId id;

    using ResponseCallbacks = std::function<void()>;

    std::unordered_map<size_t, ResponseCallbacks> callbacks;

};

#endif // MESSENGER_H
