#ifndef MANAGERWRAPPER_H
#define MANAGERWRAPPER_H

#include "CallbackCallWrapper.h"

#include "utilites/OopUtils.h"

class ManagerWrapper: public CallbackCallWrapper, public no_copyable, public no_moveable{
    Q_OBJECT
public:
    ManagerWrapper();
    virtual ~ManagerWrapper();
};

#endif // MANAGERWRAPPER_H
