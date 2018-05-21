#include "rlp.h"
#include "utils2.h"

#include <cstring>

template<typename PODType>
size_t NumberSize(const PODType* value) {
    PODType val = *value;
    size_t currVal = 0;
    while (val != 0) {
        currVal++;
        val /= 256;
    }
    return currVal;
}

std::string EncodeField(std::string field) {
    size_t fs = field.size();
    std::string rslt = "";
    if (fs == 1 && field.at(0) >= 0x0 && (uint8_t)field.at(0) <= 0x7F) {
        if (field.at(0) == 0) {
            rslt += 0x80;
        } else {
            rslt += field;
        }
    } else if (fs <= 55) {
        char sz = 0x80 + fs;
        rslt += sz;
        rslt += field;
    } else if (fs > 55 && fs < 0xFFFFFFFFFFFFFFFF) {
        size_t sizelen = NumberSize(&fs);

        const std::string bigint = IntegerToBuffer(fs);

        char prefix = 0xB7 + sizelen;
        rslt += prefix;
        rslt += bigint.substr(0, sizelen);
        rslt += field;
    }

    return rslt;
}

std::string CalcTotalSize(std::string dump) {
    std::string rslt = "";
    size_t ds = dump.size();
    if (ds <= 55) {
        char sz = 0xC0 + ds;
        rslt += sz;
        rslt += dump;
    } else {
        size_t sizelen = NumberSize(&ds);

        const std::string bigint = IntegerToBuffer(ds);

        char prefix = 0xF7 + sizelen;
        rslt += prefix;
        rslt += bigint.substr(0, sizelen);
        rslt += dump;
    }

    return rslt;
}

std::string RLP(const std::vector<std::string> fields) {
    std::string dump = "";
    for (size_t i = 0; i < fields.size(); ++i) {
        dump += EncodeField(fields[i]);
    }
    dump = CalcTotalSize(dump);
    return dump;
}
