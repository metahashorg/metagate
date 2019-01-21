#include "InitInterface.h"

#include "check.h"
#include "SlotWrapper.h"

#include <QTimer>

#include "Initializer.h"

namespace initializer {

InitInterface::InitInterface(const QString &type, QThread *mainThread, Initializer &manager, bool isTimerEnabled)
    : type(type)
    , mainThread(mainThread)
    , manager(manager)
    , isTimerEnabled(isTimerEnabled)
{
    if (isTimerEnabled) {
        CHECK(connect(&timer, SIGNAL(timeout()), this, SLOT(onTimerEvent())), "not connect timeout");
        timer.setInterval(milliseconds(1s).count());
        timer.moveToThread(mainThread);
        timer.start();
    }
}

void InitInterface::sendState(const InitState &state) {
    CHECK(state.type == type, "Type incorrect: " + state.type.toStdString() + ". Expected: " + type.toStdString());
    emit manager.sendState(state);
}

void InitInterface::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    const time_point now = ::now();
    std::vector<std::tuple<time_point, milliseconds, std::string, std::function<void(const TypedException &e)>>> toEvents;
    std::unique_lock<std::mutex> lock(eventsMut);
    for (auto iter = events.begin(); iter != events.end();) {
        const auto &tuple = *iter;
        if (now - std::get<time_point>(tuple) >= std::get<milliseconds>(tuple)) {
            toEvents.emplace_back(tuple);
            iter = events.erase(iter);
        } else {
            iter++;
        }
    }
    lock.unlock();
    if (!toEvents.empty()) {
        for (const auto &tuple: toEvents) {
            emit std::get<std::function<void(const TypedException &e)>>(tuple)(TypedException(INITIALIZER_TIMEOUT_ERROR, std::get<std::string>(tuple)));
        }
        lock.lock();
        if (events.empty()) {
            timer.stop();
        }
    }
END_SLOT_WRAPPER
}

void InitInterface::setTimerEvent(const milliseconds &timer, const std::string &errorDesc, const std::function<void(const TypedException &e)> &func) {
    CHECK(isTimerEnabled, "Timer not started");
    CHECK(this->timer.isActive(), "Timer already stopped");
    std::lock_guard<std::mutex> lock(eventsMut);
    events.emplace_back(::now(), timer, errorDesc, func);
}

}
