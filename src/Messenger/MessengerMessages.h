#ifndef MESSENGERMESSAGES_H
#define MESSENGERMESSAGES_H

#include <vector>

#include "Message.h"

class QJsonDocument;

namespace messenger {

QString makeTextForSignRegisterRequest(const QString &address, const QString &rsaPubkeyHex, uint64_t fee);

QString makeTextForSignRegisterBlockchainRequest(const QString &address, uint64_t fee, const QString &txHash, const QString &blockchain, const QString &blockchainName);

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

QString makeTextForWantTalkRequest(const QString &address);

QString makeRegisterRequest(const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, size_t id);

QString makeRegisterBlockchainRequest(const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee, const QString &txHash, const QString &blockchain, const QString &blockchainName, size_t id);

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

QString makeAddAllKeysRequest(const std::vector<QString> &wallets, size_t id);

QString makeWantToTalkRequest(const QString &toAddress, const QString &pubkey, const QString &sign, size_t id);

enum METHOD: int {
    APPEND_KEY_TO_ADDR = 0, GET_KEY_BY_ADDR = 1, SEND_TO_ADDR = 2, NEW_MSGS = 3, NEW_MSG = 4, COUNT_MESSAGES = 5,
    CHANNEL_CREATE = 6, CHANNEL_ADD_WRITER = 7, CHANNEL_DEL_WRITER = 8, SEND_TO_CHANNEL = 9, GET_CHANNEL = 10, GET_CHANNELS = 11, GET_MY_CHANNELS = 12, ADD_TO_CHANNEL = 13, DEL_FROM_CHANNEL = 14,
    ALL_KEYS_ADDED = 15, WANT_TO_TALK = 16, REQUIRES_PUBKEY = 17, COLLOCUTOR_ADDED_PUBKEY = 18,
    NOT_SET = 1000
};

struct ResponseType {
    METHOD method = METHOD::NOT_SET;

    enum class ERROR_TYPE {
        NO_ERROR, ADDRESS_EXIST, SIGN_OR_ADDRESS_INVALID, INCORRECT_JSON, ADDRESS_NOT_FOUND, CHANNEL_EXIST, CHANNEL_NOT_PERMISSION, CHANNEL_NOT_FOUND, INVALID_ADDRESS, OTHER
    };

    QString address;
    bool isError = false;
    QString error;
    ERROR_TYPE errorType = ERROR_TYPE::NO_ERROR;
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
    QString txHash;
    QString blockchain_name;
};

struct RequiresPubkeyResponse {
    QString address;
    QString collocutor;
};

struct CollocutorAddedPubkeyResponse {
    QString address;
    QString pubkey;
    QString collocutor;
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

KeyMessageResponse parsePublicKeyMessageResponse(const QJsonDocument &response);

RequiresPubkeyResponse parseRequiresPubkeyResponse(const QJsonDocument &response);

CollocutorAddedPubkeyResponse parseCollocutorAddedPubkeyResponse(const QJsonDocument &response);

}

#endif // MESSENGERMESSAGES_H
