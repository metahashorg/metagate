#ifndef DURATION_H_
#define DURATION_H_

#include <chrono>

using time_point = std::chrono::time_point<std::chrono::steady_clock>;

using system_time_point = std::chrono::time_point<std::chrono::system_clock>;

using milliseconds = std::chrono::milliseconds;
using microseconds = std::chrono::microseconds;
using seconds = std::chrono::seconds;
using hours = std::chrono::hours;
using days = std::chrono::duration<long, std::ratio_multiply<hours::period, std::ratio<24>>::type>;

using namespace std::chrono_literals;
using namespace std::literals;
using namespace std::literals::chrono_literals;

inline time_point now() {
    return std::chrono::steady_clock::now();
}

inline system_time_point system_now() {
    return std::chrono::system_clock::now();
}

inline size_t timePointToInt(const time_point &tp) {
    const milliseconds timeMs = std::chrono::duration_cast<milliseconds>(std::chrono::time_point_cast<milliseconds>(tp).time_since_epoch());
    return timeMs.count();
}

inline time_point intToTimePoint(size_t timestamp) {
    return time_point() + milliseconds(timestamp);
}

inline size_t systemTimePointToInt(const system_time_point &tp) {
    const milliseconds timeMs = std::chrono::duration_cast<milliseconds>(std::chrono::time_point_cast<milliseconds>(tp).time_since_epoch());
    return timeMs.count();
}

inline system_time_point intToSystemTimePoint(size_t timestamp) {
    return system_time_point() + milliseconds(timestamp);
}

#endif // DURATION_H_
