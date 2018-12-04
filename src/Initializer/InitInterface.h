#ifndef INITINTERFACE_H
#define INITINTERFACE_H

namespace initializer {

class Initializer;
struct InitState;

class InitInterface {
public:

    InitInterface(Initializer &manager, int fromNumber, int toNumber);

    virtual ~InitInterface() = default;

    void sendState(const InitState &state);

    virtual void complete() = 0;

protected:

    Initializer &manager;

    int fromNumber;

    int toNumber;
};

}

#endif // INITINTERFACE_H
