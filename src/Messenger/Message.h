#ifndef MESSAGE_H
#define MESSAGE_H

#include <QString>

struct Message {
    using Counter = uint64_t;

    QString collocutor;
    bool isInput;
    uint64_t timestamp;
    QString data;
    Counter counter;
    uint64_t fee;
};

#endif // MESSAGE_H
