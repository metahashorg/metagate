#ifndef MESSENGERMESSAGES_H
#define MESSENGERMESSAGES_H

#include <QJsonDocument>

#include <vector>

#include "Messenger.h"
#include "Message.h"

QString makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee);

QString makeTextForGetPubkeyRequest(const QString &address);

QString makeTextForSendMessageRequest(const QString &address, const QString &dataHex, uint64_t fee, uint64_t timestamp);

QString makeTextForGetMyMessagesRequest();

QString makeTextForChannelCreateRequest(const QString &title, const QString titleSha, uint64_t fee);

QString makeTextForChannelAddWriterRequest(const QString &titleSha, const QString &address);

QString makeTextForChannelDelWriterRequest(const QString &titleSha, const QString &address);

QString makeTextForSendToChannelRequest(const QString &titleSha, const QString &text, uint64_t fee, uint64_t timestamp);

QString makeTextForGetChannelRequest();

QString makeTextForGetChannelsRequest();

QString makeTextForGetMyChannelsRequest();

QString makeTextForMsgAppendKeyOnlineRequest();

QString makeRegisterRequest(const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, size_t id);

QString makeGetPubkeyRequest(const QString &address, const QString &pubkeyHex, const QString &signHex, size_t id);

QString makeSendMessageRequest(const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee, uint64_t timestamp, size_t id);

QString makeGetMyMessagesRequest(const QString &pubkeyHex, const QString &signHex, Message::Counter from, Message::Counter to, size_t id);

QString makeCreateChannelRequest(const QString &title, const QString &titleSha, uint64_t fee, const QString &pubkeyHex, const QString &signHex, size_t id);

QString makeChannelAddWriterRequest(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, size_t id);

QString makeChannelDelWriterRequest(const QString &titleSha, const QString &address, const QString &pubkeyHex, const QString &signHex, size_t id);

QString makeSendToChannelRequest(const QString &titleSha, const QString &dataHex, uint64_t fee, uint64_t timestamp, const QString &pubkeyHex, const QString &signHex, size_t id);

QString makeGetChannelRequest(const QString &titleSha, Message::Counter from, Message::Counter to, const QString &pubkeyHex, const QString &signHex, size_t id);

QString makeGetChannelsRequest(const QString &pubkeyHex, const QString &signHex, size_t id);

QString makeGetMyChannelsRequest(const QString &pubkeyHex, const QString &signHex, size_t id);

QString makeAppendKeyOnlineRequest(const QString &pubkeyHex, const QString &signHex, size_t id);

enum METHOD: int {
    APPEND_KEY_TO_ADDR = 0, GET_KEY_BY_ADDR = 1, SEND_TO_ADDR = 2, NEW_MSGS = 3, NEW_MSG = 4, COUNT_MESSAGES = 5,
    CHANNEL_CREATE = 6, CHANNEL_ADD_WRITER = 7, CHANNEL_DEL_WRITER = 8, SEND_TO_CHANNEL = 9, GET_CHANNEL = 10, GET_CHANNELS = 11, GET_MY_CHANNELS = 12, ADD_TO_CHANNEL = 13, DEL_FROM_CHANNEL = 14,
    NOT_SET = 1000
};

struct ResponseType {
    METHOD method = METHOD::NOT_SET;
    QString address;
    bool isError = false;
    QString error;
    size_t id = -1;
};

struct NewMessageResponse {
    QString data;
    bool isInput;
    bool isChannel;
    Message::Counter counter;
    uint64_t timestamp;
    uint64_t fee;
    QString collocutor;
    QString channelName;

    bool operator< (const NewMessageResponse &second) const {
        return this->counter < second.counter;
    }
};

struct KeyMessageResponse {
    QString publicKey;
    QString addr;
    uint64_t fee;
};

ResponseType getMethodAndAddressResponse(const QJsonDocument &response);

NewMessageResponse parseNewMessageResponse(const QJsonDocument &response);

std::vector<NewMessageResponse> parseNewMessagesResponse(const QJsonDocument &response);

std::vector<NewMessageResponse> parseGetChannelResponse(const QJsonDocument &response);

std::vector<ChannelInfo> parseGetChannelsResponse(const QJsonDocument &response);

std::vector<ChannelInfo> parseGetMyChannelsResponse(const QJsonDocument &response);

ChannelInfo parseAddToChannelResponse(const QJsonDocument &response);

ChannelInfo parseDelToChannelResponse(const QJsonDocument &response);

Message::Counter parseCountMessagesResponse(const QJsonDocument &response);

KeyMessageResponse parseKeyMessageResponse(const QJsonDocument &response);

#endif // MESSENGERMESSAGES_H
