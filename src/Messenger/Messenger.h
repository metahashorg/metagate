#ifndef MESSENGER_H
#define MESSENGER_H

#include <QObject>
#include <QVariant>

#include "qt_utilites/TimerClass.h"
#include "Network/WebSocketClient.h"

#include "utilites/RequestId.h"
#include "Message.h"

#include "qt_utilites/CallbackWrapper.h"
#include "qt_utilites/ManagerWrapper.h"

#include <map>
#include <set>
#include <unordered_map>
#include <functional>

struct TypedException;
class MainWindow;

struct MessengerDeleteFromChannelVariant {
    QString address;
    QString channelTitle;
    QString channelTitleSha;
    QString channelAdmin;

    MessengerDeleteFromChannelVariant() = default;

    MessengerDeleteFromChannelVariant(const QString &address, const QString &channelTitle, const QString &channelTitleSha, const QString &channelAdmin)
        : address(address)
        , channelTitle(channelTitle)
        , channelTitleSha(channelTitleSha)
        , channelAdmin(channelAdmin)
    {}
};

Q_DECLARE_METATYPE(MessengerDeleteFromChannelVariant);

struct MessengerAddedFromChannelVariant {
    QString address;
    QString channelTitle;
    QString channelTitleSha;
    QString channelAdmin;
    messenger::Message::Counter counter;

    MessengerAddedFromChannelVariant() = default;

    MessengerAddedFromChannelVariant(const QString &address, const QString &channelTitle, const QString &channelTitleSha, const QString &channelAdmin, messenger::Message::Counter counter)
        : address(address)
        , channelTitle(channelTitle)
        , channelTitleSha(channelTitleSha)
        , channelAdmin(channelAdmin)
        , counter(counter)
    {}
};

Q_DECLARE_METATYPE(MessengerAddedFromChannelVariant);

struct MessengerRequiresPubkeyVariant {
    QString address;
    QString collocutor;

    MessengerRequiresPubkeyVariant() = default;

    MessengerRequiresPubkeyVariant(const QString &address, const QString &collocutor)
        : address(address)
        , collocutor(collocutor)
    {}
};

Q_DECLARE_METATYPE(MessengerRequiresPubkeyVariant);

struct MessengerCollocutorAddedPubkeyVariant {
    QString address;
    QString collocutor;

    MessengerCollocutorAddedPubkeyVariant() = default;

    MessengerCollocutorAddedPubkeyVariant(const QString &address, const QString &collocutor)
        : address(address)
        , collocutor(collocutor)
    {}
};

Q_DECLARE_METATYPE(MessengerCollocutorAddedPubkeyVariant);
// TODO переписать на обычный variant

namespace messenger {

struct NewMessageResponse;
class MessengerJavascript;
class MessengerDBStorage;
struct ChannelInfo;
class CryptographicManager;

class Messenger : public ManagerWrapper, public TimerClass
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

    using GetMessagesCallback = CallbackWrapper<void(const std::vector<Message> &messages)>;

    using SavePosCallback = CallbackWrapper<void()>;

    using GetSavedPosCallback = CallbackWrapper<void(const Message::Counter &pos)>;

    using GetSavedsPosCallback = CallbackWrapper<void(const std::vector<std::pair<QString, Message::Counter>> &pos)>;

    using RegisterAddressCallback = CallbackWrapper<void(bool isNew)>;

    using RegisterAddressBlockchainCallback = CallbackWrapper<void(bool isNew)>;

    using SignedStringsCallback = CallbackWrapper<void()>;

    using SavePubkeyCallback = CallbackWrapper<void(bool isNew)>;

    using GetPubkeyAddressCallback = CallbackWrapper<void(const QString &pubkey)>;

    using SendMessageCallback = CallbackWrapper<void()>;

    using GetCountMessagesCallback = CallbackWrapper<void(const Message::Counter &count)>;

    using CreateChannelCallback = CallbackWrapper<void()>;

    using AddWriterToChannelCallback = CallbackWrapper<void()>;

    using DelWriterToChannelCallback = CallbackWrapper<void()>;

    using GetChannelListCallback = CallbackWrapper<void(const std::vector<ChannelInfo> &channels)>;

    using DecryptUserMessagesCallback = CallbackWrapper<void()>;

    using CompleteUserCallback = CallbackWrapper<void(bool isComplete)>;

    using AddAllWalletsInFolderCallback = CallbackWrapper<void()>;

    using WantToTalkCallback = CallbackWrapper<void()>;

    using UserInfoCallback = CallbackWrapper<void(bool complete, const ContactInfo &info)>;

public:

    explicit Messenger(MessengerJavascript &javascriptWrapper, MessengerDBStorage &db, CryptographicManager &cryptManager, MainWindow &mainWin, QObject *parent = nullptr);

    ~Messenger() override;

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

signals:

    void showNotification(const QString &title, const QString &message);

public:

    static std::vector<QString> stringsForSign();

    static QString makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee);

    static QString makeTextForSignRegisterBlockchainRequest(const QString &address, uint64_t fee, const QString &txHash, const QString &blockchain, const QString &blockchainName);

    static QString makeTextForGetPubkeyRequest(const QString &address);

    static QString makeTextForSendMessageRequest(const QString &address, const QString &dataHex, const QString &encryptedSelfDataHex, uint64_t fee, uint64_t timestamp);

    static QString makeTextForChannelCreateRequest(const QString &title, const QString titleSha, uint64_t fee);

    static QString makeTextForChannelAddWriterRequest(const QString &titleSha, const QString &address);

    static QString makeTextForChannelDelWriterRequest(const QString &titleSha, const QString &address);

    static QString makeTextForSendToChannelRequest(const QString &titleSha, const QString &text, uint64_t fee, uint64_t timestamp);

    static QString makeTextForWantToTalkRequest(const QString &address);


    static void checkChannelTitle(const QString &title);

    static QString getChannelSha(const QString &title);

signals:

    void registerAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const Messenger::RegisterAddressCallback &callback);

    void registerAddressFromBlockchain(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const QString &txHash, const QString &blockchain, const QString &blockchainName, const Messenger::RegisterAddressBlockchainCallback &callback);

    void savePubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex, const SavePubkeyCallback &callback);

    void getPubkeyAddress(const QString &address, const GetPubkeyAddressCallback &callback);

    void getUserInfo(const QString &address, const UserInfoCallback &callback);

    void getCollocutorInfo(const QString &address, const UserInfoCallback &callback);


    void sendMessage(const QString &thisAddress, const QString &toAddress, bool isChannel, QString channel, const QString &dataHex, const QString &decryptedDataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback);

    void signedStrings(const QString &address, const std::vector<QString> &signedHexs, const SignedStringsCallback &callback);

    void getLastMessage(const QString &address, bool isChannel, QString channel, const GetSavedPosCallback &callback);

    void getSavedPos(const QString &address, bool isChannel, const QString &collocutorOrChannel, const GetSavedPosCallback &callback);

    void getSavedsPos(const QString &address, bool isChannel, const GetSavedsPosCallback &callback);

    void savePos(const QString &address, bool isChannel, const QString &collocutorOrChannel, Message::Counter pos, const SavePosCallback &callback);

    void getCountMessages(const QString &address, const QString &collocutor, Message::Counter from, const GetCountMessagesCallback &callback);

    void getHistoryAddress(QString address, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void getHistoryAddressAddress(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void getHistoryAddressAddressCount(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter count, Message::Counter to, const GetMessagesCallback &callback);


    void createChannel(const QString &address, const QString &title, const QString &titleSha, const QString &pubkeyHex, const QString &signHex, uint64_t fee, const CreateChannelCallback &callback);

    void addWriterToChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const AddWriterToChannelCallback &callback);

    void delWriterFromChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const DelWriterToChannelCallback &callback);

    void getChannelList(const QString &address, const GetChannelListCallback &callback);


    void decryptMessages(const QString &address, const DecryptUserMessagesCallback &callback);

    void isCompleteUser(const QString &address, const CompleteUserCallback &callback);


    void addAllAddressesInFolder(const QString &folder, const std::vector<QString> &addresses, const AddAllWalletsInFolderCallback &callback);

    void wantToTalk(const QString &address, const QString &pubkey, const QString &sign, const WantToTalkCallback &callback);


    void reEmit();

private slots:

    void onRegisterAddress(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const Messenger::RegisterAddressCallback &callback);

    void onRegisterAddressFromBlockchain(bool isForcibly, const QString &address, const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const QString &txHash, const QString &blockchain, const QString &blockchainName, const Messenger::RegisterAddressBlockchainCallback &callback);

    void onSavePubkeyAddress(bool isForcibly, const QString &address, const QString &pubkeyHex, const QString &signHex, const SavePubkeyCallback &callback);

    void onGetPubkeyAddress(const QString &address, const GetPubkeyAddressCallback &callback);

    void onGetUserInfo(const QString &address, const UserInfoCallback &callback);

    void onGetCollocutorInfo(const QString &address, const UserInfoCallback &callback);


    void onSendMessage(const QString &thisAddress, const QString &toAddress, bool isChannel, QString channel, const QString &dataHex, const QString &decryptedDataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, const QString &encryptedDataHex, const SendMessageCallback &callback);

    void onSignedStrings(const QString &address, const std::vector<QString> &signedHexs, const SignedStringsCallback &callback);

    void onGetLastMessage(const QString &address, bool isChannel, QString channel, const GetSavedPosCallback &callback);

    void onGetSavedPos(const QString &address, bool isChannel, const QString &collocutorOrChannel, const GetSavedPosCallback &callback);

    void onGetSavedsPos(const QString &address, bool isChannel, const GetSavedsPosCallback &callback);

    void onSavePos(const QString &address, bool isChannel, const QString &collocutorOrChannel, Message::Counter pos, const SavePosCallback &callback);

    void onGetCountMessages(const QString &address, const QString &collocutor, Message::Counter from, const GetCountMessagesCallback &callback);

    void onGetHistoryAddress(QString address, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void onGetHistoryAddressAddress(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter from, Message::Counter to, const GetMessagesCallback &callback);

    void onGetHistoryAddressAddressCount(QString address, bool isChannel, const QString &collocutorOrChannel, Message::Counter count, Message::Counter to, const GetMessagesCallback &callback);


    void onCreateChannel(const QString &address, const QString &title, const QString &titleSha, const QString &pubkeyHex, const QString &signHex, uint64_t fee, const CreateChannelCallback &callback);

    void onAddWriterToChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const AddWriterToChannelCallback &callback);

    void onDelWriterFromChannel(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, const DelWriterToChannelCallback &callback);

    void onGetChannelList(const QString &address, const GetChannelListCallback &callback);


    void onDecryptMessages(const QString &address, const DecryptUserMessagesCallback &callback);

    void onIsCompleteUser(const QString &address, const CompleteUserCallback &callback);


    void onAddAllAddressesInFolder(const QString &folder, const std::vector<QString> &addresses, const AddAllWalletsInFolderCallback &callback);

    void onWantToTalk(const QString &address, const QString &pubkey, const QString &sign, const WantToTalkCallback &callback);


    void onReEmit();

private slots:

    void onWssMessageReceived(QString message);

private:

    void getMessagesFromAddressFromWss(const QString &fromAddress, Message::Counter from, Message::Counter to, bool missed = false);

    void getMessagesFromChannelFromWss(const QString &fromAddress, const QString &channelSha, Message::Counter from, Message::Counter to);

    void clearAddressesToMonitored();

    void addAddressToMonitored(const QString &address);

    void processMessages(const QString &address, const std::vector<NewMessageResponse> &messages, bool isChannel, size_t requestId);

    bool checkSignsAddress(const QString &address) const;

    QString getSignFromMethod(const QString &address, const QString &method) const;

    std::vector<QString> getAddresses() const;

    void processMyChannels(const QString &address, const std::vector<ChannelInfo> &channels);

    void invokeCallback(size_t requestId, const TypedException &exception);

    void processAddOrDeleteInChannel(const QString &address, const ChannelInfo &channel, bool isAdd);

private:

    bool isDecryptDataSave = false;
    int retrievedMissed = 0;
    QSet<size_t> ids;
    QSet<size_t> messageRetrieves;

    MessengerDBStorage &db;

    MessengerJavascript &javascriptWrapper;

    CryptographicManager &cryptManager;

    WebSocketClient wssClient;

    std::map<std::pair<QString, QString>, DeferredMessage> deferredMessages;

    RequestId id;

    using ResponseCallbacks = std::function<void(const TypedException &exception)>;

    std::unordered_map<size_t, ResponseCallbacks> callbacks;

    QString currentWalletFolder;

    std::map<QString, std::set<QString>> walletFolders;

    std::vector<QVariant> events;

};

}

#endif // MESSENGER_H
