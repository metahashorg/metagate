#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>

struct Message {
    using Counter = qint64;

    QString collocutor;
    bool isInput;
    quint64 timestamp;
    QString data;
    Counter counter;
    int64_t fee;
    bool isCanDecrypted = true;
    bool isConfirmed = true;
};

#endif // MESSAGE_H
