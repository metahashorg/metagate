#include "VersionWrapper.h"

#include <cstdlib>

#include "check.h"

Version::Version(const std::string &str) {
    const size_t found1 = str.find(".");
    CHECK(found1 != str.npos, "Incorrect version " + str);
    const std::string v1Str = str.substr(0, found1);
    const size_t found2 = str.find(".", found1 + 1);
    CHECK(found2 != str.npos, "Incorrect version " + str);
    const std::string v2Str = str.substr(found1 + 1, found2 - found1 - 1);
    const std::string v3Str = str.substr(found2 + 1);

    v1 = std::stoi(v1Str);
    v2 = std::stoi(v2Str);
    v3 = std::stoi(v3Str);

    CHECK(makeVersion(v1, v2, v3) == str, "Incorrect version " + str);
}

std::string Version::makeVersion(int v1, int v2, int v3) {
    return std::to_string(v1) + "." + std::to_string(v2) + "." + std::to_string(v3);
}

std::string Version::makeStr() const {
    return makeVersion(v1, v2, v3);
}
