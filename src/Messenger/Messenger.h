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

struct TypedException;

namespace messenger {

struct NewMessageResponse;
class MessengerJavascript;
class MessengerDBStorage;
struct ChannelInfo;

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

    using GetMessagesCallback = std::function<void(const std::vector<Message> &messages, const TypedException &exception)>;

    using SavePosCallback = std::function<void(const TypedException &exception)>;

    using GetSavedPosCallback = std::function<void(const Message::Counter &pos, const TypedException &exception)>;

    using GetSavedsPosCallback = std::function<void(const std::vector<std::pair<QString, Message::Counter>> &pos, const TypedException &exception)>;

    using RegisterAddressCallback = std::function<void(bool isNew, const TypedException &exception)>;

    using SignedStringsCallback = std::function<void(const TypedException &exception)>;

    using SavePubkeyCallback = std::function<void(bool isNew, const TypedException &exception)>;

    using GetPubkeyAddressCallback = std::function<void(const QString &pubkey, const TypedException &exception)>;

    using SendMessageCallback = std::function<void(const TypedException &exception)>;

    using GetCountMessagesCallback = std::function<void(const Message::Counter &count, const TypedException &exception)>;

    using CreateChannelCallback = std::function<void(const TypedException &exception)>;

    using AddWriterToChannelCallback = std::function<void(const TypedException &exception)>;

    using DelWriterToChannelCallback = std::function<void(const TypedException &exception)>;

    using GetChannelListCallback = std::function<void(const std::vector<ChannelInfo> &channels, const TypedException &exception)>;

public:

    explicit Messenger(MessengerJavascript &javascriptWrapper, MessengerDBStorage &db, QObject *parent = nullptr);

public:

    static std::vector<QString> stringsForSign();

    static QString makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee);

    static QString makeTextForGetPubkeyRequest(const QString &address);

    static QString makeTextForSendMessageRequest(const QString &address, const QString &dataHex, uint64_t fee, uint64_t timestamp);

    static QString makeTextForChannelCreateRequest(const QString &title, const QString titleSha, uint64_t fee);

    static QString makeTextForChannelAddWriterRequest(const QString &titleSha, const QString &address);

    static QString makeTextForChannelDelWriterRequest(const QString &titleSha, const QString &address);

    static QString makeTextForSendToChannelRequest(const QString &titleSha, const QString &text, uint64_t fee, uint64_t timestamp);


    static void checkChannelTitle(const QString &title);

    static QString getChannelSha(const QString &title);

signals:

    void registerAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const RegisterAddressCallback &callback);

    void savePubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex, const SavePubkeyCallback &callback);

    void getPubkeyAddress(const QString &address, const GetPubkeyAddressCallback &callback);

    void sendMessage(const QString &thisAddress, const QString &toAddress, bool isChannel, QString channel, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback);

    void signedStrings(const QString &address, const std::vector<QString> &signedHexs, const SignedStringsCallback &callback);

    void getLastMessage(const QString &address, bool isChannel, QString channel, const GetSavedPosCallback &callback);

    void getSavedPos(const QString &address, bool isChannel, const QString &collocutorOrChannel, const GetSavedPosCallback &callback);

    void getSavedsPos(const QString &address, bool isChannel, const GetSavedsPosCallback &callback);

    void savePos(const QString &address, bool isChannel, const QString &collocutorOrChannel, Message::Counter pos, const SavePosCallback &callback);

    void getCountMessages(const QString &address, const QString &collocutor, Message::Counter from, const GetCountMessagesCallback &callback);

    void getHistoryAddress(QString address, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void getHistoryAddressAddress(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void getHistoryAddressAddressCount(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter count, Message::Counter to, const GetMessagesCallback &callback);


    void createChannel(const QString &title, const QString &titleSha, const QString &pubkeyHex, const QString &signHex, uint64_t fee, const CreateChannelCallback &callback);

    void addWriterToChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const AddWriterToChannelCallback &callback);

    void delWriterFromChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const DelWriterToChannelCallback &callback);

    void getChannelList(const QString &address, const GetChannelListCallback &callback);

private slots:

    void onRegisterAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const RegisterAddressCallback &callback);

    void onSavePubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex, const SavePubkeyCallback &callback);

    void onGetPubkeyAddress(const QString &address, const GetPubkeyAddressCallback &callback);

    void onSendMessage(const QString &thisAddress, const QString &toAddress, bool isChannel, QString channel, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback);

    void onSignedStrings(const QString &address, const std::vector<QString> &signedHexs, const SignedStringsCallback &callback);

    void onGetLastMessage(const QString &address, bool isChannel, QString channel, const GetSavedPosCallback &callback);

    void onGetSavedPos(const QString &address, bool isChannel, const QString &collocutorOrChannel, const GetSavedPosCallback &callback);

    void onGetSavedsPos(const QString &address, bool isChannel, const GetSavedsPosCallback &callback);

    void onSavePos(const QString &address, bool isChannel, const QString &collocutorOrChannel, Message::Counter pos, const SavePosCallback &callback);

    void onGetCountMessages(const QString &address, const QString &collocutor, Message::Counter from, const GetCountMessagesCallback &callback);

    void onGetHistoryAddress(QString address, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void onGetHistoryAddressAddress(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void onGetHistoryAddressAddressCount(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter count, Message::Counter to, const GetMessagesCallback &callback);


    void onCreateChannel(const QString &title, const QString &titleSha, const QString &pubkeyHex, const QString &signHex, uint64_t fee, const CreateChannelCallback &callback);

    void onAddWriterToChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const AddWriterToChannelCallback &callback);

    void onDelWriterFromChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const DelWriterToChannelCallback &callback);

    void onGetChannelList(const QString &address, const GetChannelListCallback &callback);

private slots:

    void onRun();

    void onTimerEvent();

    void onWssMessageReceived(QString message);

private:

    void getMessagesFromAddressFromWss(const QString &fromAddress, Message::Counter from, Message::Counter to);

    void getMessagesFromChannelFromWss(const QString &fromAddress, const QString &channelSha, Message::Counter from, Message::Counter to);

    void clearAddressesToMonitored();

    void addAddressToMonitored(const QString &address);

    void processMessages(const QString &address, const std::vector<NewMessageResponse> &messages, bool isChannel);

    QString getSignFromMethod(const QString &address, const QString &method) const;

    std::vector<QString> getMonitoredAddresses() const;

    void processMyChannels(const QString &address, const std::vector<ChannelInfo> &channels);

    void invokeCallback(size_t requestId, const TypedException &exception);

    void processAddOrDeleteInChannel(const QString &address, const ChannelInfo &channel, bool isAdd);

private:

    MessengerDBStorage &db;

    MessengerJavascript &javascriptWrapper;

    WebSocketClient wssClient;

    std::map<std::pair<QString, QString>, DeferredMessage> deferredMessages;

    RequestId id;

    using ResponseCallbacks = std::function<void(const TypedException &exception)>;

    std::unordered_map<size_t, ResponseCallbacks> callbacks;

};

}

#endif // MESSENGER_H
