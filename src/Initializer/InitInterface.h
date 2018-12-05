#ifndef INITINTERFACE_H
#define INITINTERFACE_H

class QThread;

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

    QThread *mainThread;

    Initializer &manager;

};

}

#endif // INITINTERFACE_H
