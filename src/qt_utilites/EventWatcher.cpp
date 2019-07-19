#include "EventWatcher.h"

#include "TypedException.h"

WatchedEvent::WatchedEvent() = default;

WatchedEvent::~WatchedEvent() = default;

void WatchedEvent::setFields(const std::string &name, size_t id, const time_point &startTime, const milliseconds &timeout) {
    this->name = name;
    this->id = id;
    this->startTime = startTime;
    this->timeout = timeout;
}

void WatchedEvent::callTimeout() {
    callError(TypedException(TypeErrors::EVENT_WATCHER_TIMEOUT, "Event " + name + " timeout"));
}

bool WatchedEvent::isTimeout(const time_point &now) const {
    return now >= startTime + timeout;
}

void WatchedEvent::removeLater() {
    isRemoved = true;
}

EventWatcher::EventWatcher() = default;

void EventWatcher::addEvent(const std::string &name, std::unique_ptr<WatchedEvent> &&event, const milliseconds &timeout) {
    const time_point now = ::now();
    const size_t currId = ++id;

    event->setFields(name, currId, now, timeout);
    events.emplace(currId, std::move(event));
}

void EventWatcher::checkEvents() {
    const time_point now = ::now();

    for (auto iter = events.begin(); iter != events.end();) {
        if (iter->second->isRemoved) {
            iter = events.erase(iter);
        } else if (iter->second->isTimeout(now)) {
            iter->second->callTimeout();
            iter = events.erase(iter);
        } else {
            iter++;
        }
    }
}

void EventWatcher::removeEvent(size_t id) {
    events.erase(id);
}
