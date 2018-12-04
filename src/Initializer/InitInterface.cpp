#include "InitInterface.h"

#include "check.h"
#include "Initializer.h"

namespace initializer {

InitInterface::InitInterface(QThread *mainThread, Initializer &manager, int fromNumber, int toNumber)
    : mainThread(mainThread)
    , manager(manager)
    , fromNumber(fromNumber)
    , toNumber(toNumber)
{}

void InitInterface::sendState(const InitState &state) {
    CHECK(fromNumber <= state.number && state.number < toNumber, "Number incorrect from state " + state.type.toStdString());
    manager.sendState(state);
}

}
