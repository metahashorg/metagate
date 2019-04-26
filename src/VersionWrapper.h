#ifndef VERSIONWRAPPER_H
#define VERSIONWRAPPER_H

#include <string>

class Version {
public:

    Version() = default;

    explicit Version(const std::string &str);

    static std::string makeVersion(int v1, int v2, int v3);

    std::string makeStr() const;

    bool operator<=(const Version &second) const;

private:

    int v1 = 0;
    int v2 = 0;
    int v3 = 0;

};

class VersionVar {
public:

    VersionVar() = default;

    explicit VersionVar(const std::string &str);

    static std::string makeVersion(int v1, int v2, int v3);

    std::string makeStr() const;

    bool operator<=(const VersionVar &second) const;

    bool operator<(const VersionVar &second) const;

private:

    int v1 = 0;
    int v2 = 0;
    int v3 = 0;

};

#endif // VERSIONWRAPPER_H
