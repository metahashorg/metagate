#ifndef VERSIONWRAPPER_H
#define VERSIONWRAPPER_H

#include <string>

class Version {
public:

    Version() = default;

    Version(const std::string &str);

    static std::string makeVersion(int v1, int v2, int v3);

    std::string makeStr() const;

    bool operator<=(const Version &second) const {
        if (v1 != second.v1) {
            return v1 < second.v1;
        } else if (v2 != second.v2) {
            return v2 < second.v2;
        } else {
            return v3 <= second.v3;
        }
    }

private:

    int v1 = 0;
    int v2 = 0;
    int v3 = 0;

};

#endif // VERSIONWRAPPER_H
