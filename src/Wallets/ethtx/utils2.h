#ifndef ETHTX_UTILS
#define ETHTX_UTILS

#include <string>
#include <vector>

std::string DumpToHexString(const uint8_t* dump, uint32_t dumpsize);
std::string DumpToHexString(const std::string &str);
std::string HexStringToDump(const std::string& hexstr);
std::string SettingsToRLP(std::vector<std::string>& strings, bool adddefault = true);
std::string ReadFile(const std::string &privKeyPath);
std::string PackInteger(uint64_t value);
std::string IntToRLP(int val);

template <typename IntType>
std::string IntegerToBuffer(IntType val)
{
    uint8_t buf[sizeof(IntType)] = {0};
    IntType value = val;
    uint8_t rem = 0;
    for (size_t i = 0; i < sizeof(IntType); ++i)
    {
        rem = value % 256;
        buf[i] = rem;
        value = value / 256;
    }
    return std::string((char*)buf, sizeof(IntType));
}

#endif
