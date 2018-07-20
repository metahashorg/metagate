#ifndef WIF_H_
#define WIF_H_

#include <string>

std::string WIFToPrivkey(const std::string& wif, bool& isCompressed);
std::string PrivKeyToPubKey(const std::string& rawprivkey);
std::string PubkeyToAddress(const std::string& rawpubkey, bool testnet);
std::string CompressedPubkeyToAddress(const std::string& rawpubkey, bool testnet);
std::string AddressToPubkeyScript(const std::string& address, bool isDecode=true);
std::string CreateWIF(bool isTestnet, bool isCompressed);
std::string PrivKeyToCompressedPubKey(const std::string& rawprivkey);

void checkAddressBase56(const std::string &address);

std::string getAddress(const std::string &wif, bool &isCompressed, bool isTestnet);

std::string encryptWif(const std::string &wif, const std::string &normalizedPassphraze);
std::string decryptWif(const std::string &encryptedWif, const std::string &normalizedPassphraze);

#endif // WIF_H_
