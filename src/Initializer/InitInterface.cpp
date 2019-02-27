#include "InitInterface.h"

#include "check.h"
#include "SlotWrapper.h"

#include <functional>
using namespace std::placeholders;

#include <QTimer>

#include "Initializer.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

InitInterface::StateType::StateType(bool isCritical, const QString &message, bool isTimeout, bool isOneRun)
    : isCritical(isCritical)
    , message(message)
    , isTimeout(isTimeout)
    , isOneRun(isOneRun)
{}

InitInterface::InitInterface(const QString &type, QThread *mainThread, Initializer &manager, bool isTimerEnabled)
    : type(type)
    , mainThread(mainThread)
    , manager(manager)
    , isTimerEnabled(isTimerEnabled)
{
    if (isTimerEnabled) {
        CHECK(connect(&timer, &QTimer::timeout, this, &InitInterface::onTimerEvent), "not connect timeout");
        timer.setInterval(milliseconds(1s).count());
        timer.moveToThread(mainThread);
        timer.start();
    }
}

void InitInterface::registerStateType(const QString &subType, const QString &message, bool isCritical, bool isOneRun, bool isTimeout, const milliseconds &timer, const std::string &errorDesc) {
    CHECK(states.find(subType) == states.end(), "Subtype " + subType.toStdString() + " already exist");
    states.emplace(subType, StateType(isCritical, message, isTimeout, isOneRun));
    if (isTimeout) {
        setTimerEvent(timer, errorDesc, std::bind(&InitInterface::sendStateTimeout, this, subType, _1));
    }
}

void InitInterface::registerStateType(const QString &subType, const QString &message, bool isCritical, bool isOneRun) {
    registerStateType(subType, message, isCritical, isOneRun, false, 0ms, "");
}

void InitInterface::registerStateType(const QString &subType, const QString &message, bool isCritical, bool isOneRun, const milliseconds &timer, const std::string &errorDesc) {
    registerStateType(subType, message, isCritical, isOneRun, true, timer, errorDesc);
}

void InitInterface::sendStateTimeout(const QString &subType, const TypedException &exception) {
    CHECK(states.find(subType) != states.end(), "SubType not found: " + subType.toStdString());
    StateType &stateType = states.at(subType);
    if (stateType.isTimeoutSended || stateType.isSended) {
        return;
    }
    stateType.isSuccess = false;
    stateType.isTimeoutSended = true;
    emit manager.sendState(InitState(type, subType, stateType.message, stateType.isCritical, false, exception));
}

void InitInterface::sendState(const QString &subType, bool isScipped, const TypedException &exception) {
    CHECK(states.find(subType) != states.end(), "SubType not found: " + subType.toStdString());
    StateType &stateType = states.at(subType);
    if (stateType.isTimeoutSended) {
        return;
    }
    if (stateType.isOneRun) {
        CHECK(!stateType.isSended, "Subtype " + subType.toStdString() + " already sended");
    } else {
        if (stateType.isSended) {
            return;
        }
    }
    stateType.isSuccess = !exception.isSet();
    stateType.isSended = true;
    emit manager.sendState(InitState(type, subType, stateType.message, stateType.isCritical, isScipped, exception));
}

void InitInterface::complete() {
    if (isCompleted) {
        return;
    }
    for (const auto &pair: states) {
        const StateType &stateType = pair.second;
        if (stateType.isCritical) {
            CHECK((stateType.isSended || stateType.isTimeoutSended) && stateType.isSuccess, "State " + type.toStdString() + ":" + pair.first.toStdString() + " not emitted or not success");
        }
    }
    completeImpl();
    isCompleted = true;
}

QString InitInterface::getType() const {
    return type;
}

std::vector<std::tuple<QString, QString, bool>> InitInterface::getSubtypes() const {
    std::vector<std::tuple<QString, QString, bool>> result;
    std::transform(states.begin(), states.end(), std::back_inserter(result), [](const auto &pair) {
        return std::make_tuple(pair.first, pair.second.message, pair.second.isCritical);
    });
    return result;
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
    CHECK(isTimerEnabled, "Init Timer not started");
    CHECK(this->timer.isActive(), "Init Timer already stopped");
    std::lock_guard<std::mutex> lock(eventsMut);
    events.emplace_back(::now(), timer, errorDesc, func);
}

}
