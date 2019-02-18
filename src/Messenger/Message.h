#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>

namespace messenger {

struct Message {
    using Counter = qint64;
    QString username = QString("");
    QString collocutor = QString("");
    bool isInput;
    quint64 timestamp;
    QString dataHex = QString("");
    QString decryptedDataHex = QString("");
    QString hash = QString("");
    Counter counter;
    int64_t fee;
    bool isCanDecrypted = true;
    bool isConfirmed = true;
    bool isChannel = false;
    bool isDecrypted = false;
    QString channel = QString("");

    bool operator< (const Message &second) const {
        return this->counter < second.counter;
    }
};

struct ChannelInfo {
    QString title = QString("");
    QString titleSha = QString("");
    QString admin = QString("");
    uint64_t fee;
    Message::Counter counter = -1;
    bool isWriter;
};

struct ContactInfo {
    QString pubkeyRsa = QString("");
    QString txRsaHash = QString("");
    QString blockchainName = QString("");
};

}

#endif // MESSAGE_H
