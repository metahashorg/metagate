#ifndef MANAGERWRAPPER_H
#define MANAGERWRAPPER_H

#include "CallbackCallWrapper.h"

class ManagerWrapper: public CallbackCallWrapper{
    Q_OBJECT
public:
    ManagerWrapper();
    virtual ~ManagerWrapper();
};

#endif // MANAGERWRAPPER_H
