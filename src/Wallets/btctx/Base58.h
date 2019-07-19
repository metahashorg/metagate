#ifndef BASE58_H
#define BASE58_H

#include <string>
#include <vector>

std::string EncodeBase58BTC(const unsigned char* pbegin, const unsigned char* pend);
bool DecodeBase58(const char* psz, std::vector<unsigned char>& vch);

#endif
