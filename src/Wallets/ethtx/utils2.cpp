#include <fstream>

#include "utils2.h"
#include "rlp.h"
#include "const.h"

#include <cstring>

#include "check.h"

std::string DumpToHexString(const uint8_t* dump, uint32_t dumpsize)
{
    std::string res;
    const char hex[] = "0123456789abcdef";
    for (size_t i = 0; i < dumpsize; ++i)
    {
        unsigned char c = static_cast<unsigned char>(dump[i]);
        res += hex[c >> 4];
        res += hex[c & 0xf];
    }
    return res;
}

std::string DumpToHexString(const std::string &str)
{
    return DumpToHexString((const uint8_t*)str.data(), str.size());
}

std::string HexStringToDump(const std::string& hexstr)
{
    std::string decoded = "";
    unsigned char a,b,c;
    a = b = c = 0;

    for (int cnt = (int)hexstr.size()-1; cnt >= 0;)
    {
        a = hexstr.at(cnt);
        --cnt;
        b = (cnt >= 0) ? hexstr.at(cnt) : '0';
        --cnt;
        if (a >= '0' && a <= '9') {
            a = a - '0';
        } else if (a >= 'a' && a <= 'f') {
            a = a - 'a' + 10;
        } else if (a >= 'A' && a <= 'F') {
            a = a - 'A' + 10;
        } else {
            throwErr("Incorrect hex str " + hexstr);
        }

        if (b >= '0' && b <= '9') {
            b = b - '0';
        } else if (b >= 'a' && b <= 'f') {
            b = b - 'a' + 10;
        } else if (b >= 'A' && b <= 'F') {
            b = b - 'A' + 10;
        } else {
            throwErr("Incorrect hex str " + hexstr);
        }

        c = (b << 4) + a;
        decoded.insert(decoded.begin(), c);
    }

    return decoded;
}

std::string IntToRLP(int val) {
    if (val == 0)
        return std::string(1, '\x00');
    uint8_t rlpval[sizeof(val)];
    unsigned char* valptr = (unsigned char*)&val + sizeof(val) - 1;

    size_t j = 0;
    bool start = false;
    for (size_t i = 0; i < sizeof(val); ++i)
    {
        if (*(valptr-i))
            start = true;
        if (start)
            rlpval[j++] = *(valptr-i);
    }

    return std::string((const char*)rlpval, j);
}

std::string SettingsToRLP(std::vector<std::string>& fields, bool adddefault)
{
    if (adddefault)
    {
        std::string hexstr = "";
        hexstr += (char)1;
        fields.push_back(hexstr);
        fields.push_back("");
        fields.push_back("");
    }

    return RLP(fields);
}

std::string ReadFile(const std::string &privKeyPath) {
    std::string result = "";
    std::ifstream file(privKeyPath.c_str(), std::ios::in);
    CHECK(file, "file " + privKeyPath + " not found");
    file.seekg(0, std::ios::end);
    size_t filesize = file.tellg();
    file.seekg(0, std::ios::beg);
    if (filesize != 0) {
        std::vector<char> content;
        content.resize(filesize+1);
        file.read(content.data(), filesize);
        if (file) {
            content[filesize] = '\0';
            result = std::string(content.data(), filesize+1);
        }
    }
    return result;
}


std::string PackInteger(uint64_t value)
{
    std::string varint = "";
    uint8_t buf[9] = {0};
    std::string bigint = "";
    if (value <= 252)
    {
        buf[0] = (uint8_t)value;
        varint = std::string((char*)buf, 1);
    }
    else
    {
        if (value >= 253 && value <= 0xFFFF)
        {
            buf[0] = 0xFD;
            uint16_t val = (uint16_t)value;
            bigint = IntegerToBuffer(val);
            memcpy(buf+1, bigint.data(), sizeof(uint16_t));
            varint = std::string((char*)buf, 3);
        }
        else
        {
            if (value >= 0x10000 && value <= 0xFFFFFFFF)
            {
                buf[0] = 0xFE;
                uint32_t val = (uint32_t)value;
                bigint = IntegerToBuffer(val);
                memcpy(buf+1, bigint.data(), sizeof(uint32_t));
                varint = std::string((char*)buf, 5);
            }
            else
            {
                if (value >= 0x100000000 && value <= 0xFFFFFFFFFFFFFFFF)
                {
                    buf[0] = 0xFF;
                    bigint = IntegerToBuffer(value);
                    memcpy(buf+1, bigint.data(), sizeof(uint64_t));
                    varint = std::string((char*)buf, 9);
                }
            }
        }
    }
    return varint;
}
