#ifndef EVENTWATCHER_H
#define EVENTWATCHER_H

#include <cstddef>
#include <unordered_map>
#include <memory>
#include <string>

#include "TypedException.h"

#include "duration.h"

class WatchedEvent {
    friend class EventWatcher;
public:

    WatchedEvent();

    virtual ~WatchedEvent();

protected:

    virtual void callError(const TypedException &excetption) = 0;

    void removeLater();

private:

    void setFields(const std::string &name, size_t id, const time_point &startTime, const milliseconds &timeout);

    void callTimeout();

    bool isTimeout(const time_point &now) const;

private:

    size_t id = 0;

    time_point startTime;
    milliseconds timeout;

    std::string name;

    bool isRemoved = false;
};

class EventWatcher {
public:

    EventWatcher();

    void addEvent(const std::string &name, std::unique_ptr<WatchedEvent> &&event, const milliseconds &timeout);

    void checkEvents();

    void removeEvent(size_t id);

private:

    std::unordered_map<size_t, std::unique_ptr<WatchedEvent>> events;

    size_t id = 0;
};

#endif // EVENTWATCHER_H
