#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>

namespace messenger {

struct Message {
    using Counter = qint64;
    QString username;
    QString collocutor;
    bool isInput;
    quint64 timestamp;
    QString data;
    QString hash;
    Counter counter;
    int64_t fee;
    bool isCanDecrypted = true;
    bool isConfirmed = true;
    bool isChannel = false;
    QString channel;
};

struct ChannelInfo {
    QString title;
    QString titleSha;
    QString admin;
    uint64_t fee;
    Message::Counter counter = -1;
    bool isWriter;
};

}

#endif // MESSAGE_H
