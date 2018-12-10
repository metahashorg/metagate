#ifndef INITINTERFACE_H
#define INITINTERFACE_H

#include "duration.h"

#include <functional>

#include <string>

class QThread;

struct TypedException;

namespace initializer {

class Initializer;
struct InitState;

class InitInterface {
public:

    InitInterface(QThread *mainThread, Initializer &manager);

    virtual ~InitInterface() = default;

    void sendState(const InitState &state);

    virtual void complete() = 0;

protected:

    // Вызывать только из конструктора, не из стороннего треда
    void setTimerEvent(const milliseconds &timer, const std::string &errorDesc, const std::function<void(const TypedException &e)> &func);

protected:

    QThread *mainThread;

    Initializer &manager;

private:

    bool eventTimerSetted = false;

};

}

#endif // INITINTERFACE_H
