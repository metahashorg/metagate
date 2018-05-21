#ifndef DURATION_H_
#define DURATION_H_

#include <chrono>

using time_point = std::chrono::time_point<std::chrono::steady_clock>;

using milliseconds = std::chrono::milliseconds;
using microseconds = std::chrono::microseconds;
using seconds = std::chrono::seconds;
using hours = std::chrono::hours;

using namespace std::chrono_literals;
using namespace std::literals;
using namespace std::literals::chrono_literals;

inline time_point now() {
    return std::chrono::steady_clock::now();
}

#endif // DURATION_H_
