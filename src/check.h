#ifndef CHECK_H_
#define CHECK_H_

#include <string>
#include "TypedException.h"

struct Exception {
    Exception(const std::string &message, const std::string &file)
        : message(message)
        , file(file)
    {}

    std::string message;

    std::string file;
};

#define throwErr(s) { \
{ \
    throw ::Exception(s + std::string(". Error at file ") \
        + std::string(__FILE__) + std::string(" line ") + std::to_string(__LINE__), std::string(__FILE__)); \
} \
}

#define CHECK(v, s) { \
if (!(v)) { \
    throwErr(s); \
} \
}

#endif // CHECK_H_
