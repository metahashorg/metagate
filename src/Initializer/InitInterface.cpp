#include "InitInterface.h"

#include "check.h"
#include "Initializer.h"

namespace initializer {

InitInterface::InitInterface(QThread *mainThread, Initializer &manager)
    : mainThread(mainThread)
    , manager(manager)
{}

void InitInterface::sendState(const InitState &state) {
    emit manager.sendState(state);
}

}
