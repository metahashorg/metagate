#ifndef MESSENGERMESSAGES_H
#define MESSENGERMESSAGES_H

#include <QJsonDocument>

#include <vector>

#include "Messenger.h"

const extern QString MSG_GET_MY_REQUEST;
const extern QString MSG_GET_CHANNEL_REQUEST;
const extern QString MSG_GET_CHANNELS_REQUEST;
const extern QString MSG_APPEND_KEY_ONLINE_REQUEST;

QString makeRegisterRequest(const QString &rsaPubkeyHex, const QString &pubkeyAddressHex, const QString &signHex, uint64_t fee);

QString makeGetPubkeyRequest(const QString &address, const QString &pubkeyHex, const QString &signHex);

QString makeSendMessageRequest(const QString &toAddress, const QString &dataHex, const QString &pubkeyHex, const QString &signHex, uint64_t fee);

QString makeGetMyMessagesRequest(const QString &pubkeyHex, const QString &signHex, Messenger::Counter from, Messenger::Counter to);

QString makeAppendKeyOnlineRequest(const QString &pubkeyHex, const QString &signHex);

enum class METHOD: int {
    APPEND_KEY_TO_ADDR = 0, GET_KEY_BY_ADDR = 1, SEND_TO_ADDR = 2, NEW_MSGS = 3, NEW_MSG = 4, COUNT_MESSAGES = 5
};

struct ResponseType {
    METHOD method;
    QString address;
    bool isError;
    QString error;
};

struct NewMessageResponse {
    QString data;
    bool isInput;
    Messenger::Counter counter;

    bool operator< (const NewMessageResponse &second) const {
        return this->counter < second.counter;
    }
};

ResponseType getMethodAndAddressResponse(const QJsonDocument &response);

NewMessageResponse parseNewMessageResponse(const QJsonDocument &response);

std::vector<NewMessageResponse> parseNewMessagesResponse(const QJsonDocument &response);

Messenger::Counter parseCountMessagesResponse(const QJsonDocument &response);

std::pair<QString, QString> parseKeyMessageResponse(const QJsonDocument &response);

#endif // MESSENGERMESSAGES_H
