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
    qRegister1<type>(name, filePos); \
    }

#define Q_REG2(type, name, isControl) { \
    const std::string filePos = std::string(__FILE__) + std::string(":") + std::to_string(__LINE__); \
    qRegister1<type>(name, filePos, isControl); \
    }

#define Q_REG3(type, name, tag) { \
    qRegister1<type>(name, tag); \
    }

#endif // QREGISTER_H
