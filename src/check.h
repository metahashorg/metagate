#ifndef CHECK_H_
#define CHECK_H_

#include <string>

using Exception = std::string;

#define throwErr(s) { \
{ \
    std::string result_ = s + std::string(". Error at file ") \
        + std::string(__FILE__) + std::string(" line ") + std::to_string(__LINE__); \
    throw ::Exception(result_); \
} \
}

#define CHECK(v, s) { \
if (!(v)) { \
    throwErr(s); \
} \
}

#endif // CHECK_H_
