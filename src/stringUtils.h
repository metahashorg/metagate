#ifndef STRING_UTILS_H_
#define STRING_UTILS_H_

#include <string>
#include <algorithm>
#include <cctype>

inline std::string ltrim(const std::string &s) {
    std::string copy(s);
    copy.erase(copy.begin(), std::find_if(copy.begin(), copy.end(), std::not1(std::ptr_fun<int, int>(std::isspace))));
    return copy;
}

inline std::string rtrim(const std::string &s) {
    std::string copy(s);
    copy.erase(std::find_if(copy.rbegin(), copy.rend(), std::not1(std::ptr_fun<int, int>(std::isspace))).base(), copy.end());
    return copy;
}

inline std::string trim(const std::string &s) {
    return ltrim(rtrim(s));
}

inline bool beginWith(const std::string &first, const std::string &second) {
    return first.find(second) == 0;
}

inline std::string toLower(const std::string &str) {
    std::string toLowerStr;
    toLowerStr.reserve(str.size() + 1);
    std::transform(str.begin(), str.end(), std::back_inserter(toLowerStr), tolower);
    return toLowerStr;
}

inline std::string toUpper(const std::string &str) {
    std::string toUpperStr;
    toUpperStr.reserve(str.size() + 1);
    std::transform(str.begin(), str.end(), std::back_inserter(toUpperStr), toupper);
    return toUpperStr;
}

#endif // STRING_UTILS_H_
