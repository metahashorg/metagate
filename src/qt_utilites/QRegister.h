#ifndef QREGISTER_H
#define QREGISTER_H

#include <string>

#include <QMetaType>

void addToSet(const std::string &name, const std::string &tag, int id, bool isControl);

template<typename T>
void qRegister1(const std::string &name, const std::string &tag, bool isControl=true) {
    const int id = qRegisterMetaType<T>(name.c_str());
    addToSet(name, tag, id, isControl);
}

#define Q_REG(type, name) { \
    const std::string filePos = std::string(__FILE__) + std::string(":") + std::to_string(__LINE__); \
    CHECK(name == std::string(#type), "Ups " + std::string(name) + " " + std::string(#type)); \
    qRegister1<type>(name, filePos); \
}

#define Q_REG2(type, name, isControl) { \
    const std::string filePos = std::string(__FILE__) + std::string(":") + std::to_string(__LINE__); \
    CHECK(name == std::string(#type), "Ups " + std::string(name) + " " + std::string(#type)); \
    qRegister1<type>(name, filePos, isControl); \
}

#define Q_REG3(type, name, tag) { \
    qRegister1<type>(name, tag); \
}

#define Q_CONNECT(from, sig, to, slot) { \
    CHECK(connect(from, sig, to, slot, Qt::UniqueConnection), "not connect signal " #sig " to slot " #slot); \
}

#define Q_CONNECT2(from, sig, to, slot, flags) { \
    CHECK(connect(from, sig, to, slot, static_cast<Qt::ConnectionType>(flags | Qt::UniqueConnection)), "not connect signal " #sig " to slot " #slot); \
}

#define Q_CONNECT3(from, sig, lambda) { \
    CHECK(connect(from, sig, lambda), "not connect signal " #sig " to lambda"); \
}

#define Q_CONNECT4(from, sig, context, lambda) { \
    CHECK(connect(from, sig, context, lambda), "not connect signal " #sig " to lambda"); \
}

#endif // QREGISTER_H
