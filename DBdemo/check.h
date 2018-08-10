#ifndef CHECK_H_
#define CHECK_H_

#include <string>
#include "TypedException.h"

using Exception = std::string;

#define throwErr(s) { \
{ \
    throw ::Exception(s + std::string(". Error at file ") \
        + std::string(__FILE__) + std::string(" line ") + std::to_string(__LINE__)); \
} \
}

#define CHECK(v, s) { \
if (!(v)) { \
    throwErr(s); \
} \
}

#endif // CHECK_H_
