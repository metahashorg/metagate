#include "InitInterface.h"

#include "check.h"

#include <QTimer>

#include "Initializer.h"

namespace initializer {

InitInterface::InitInterface(QThread *mainThread, Initializer &manager)
    : mainThread(mainThread)
    , manager(manager)
{}

void InitInterface::sendState(const InitState &state) {
    emit manager.sendState(state);
}

void InitInterface::setTimerEvent(const milliseconds &timer, const std::string &errorDesc, const std::function<void(const TypedException &e)> &func) {
    CHECK(!eventTimerSetted, "Only 1 event timeout supported");
    QTimer::singleShot(timer.count(), std::bind(func, TypedException(INITIALIZER_TIMEOUT_ERROR, errorDesc)));
    eventTimerSetted = true;
}

}
