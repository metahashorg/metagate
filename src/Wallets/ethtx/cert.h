#ifndef ETHTX_SIGN
#define ETHTX_SIGN

#include <cryptopp/eccrypto.h>

struct CertParams
{
    int version;
    //Параметры для aes
    std::string cipher;
    std::string ciphertext;
    std::string iv;
    //Параметры для наследования ключа PBKDF2
    std::string kdftype;
    int dklen;
    int n;
    int p;
    int r;
    std::string salt;
    std::string mac;
    std::string address;
};

CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey DecodeCert(const char* certContent, const std::string& pass, uint8_t* rawkey);
std::pair<std::string, std::string> CreateNewKey(const std::string& password);
std::string AddressFromPrivateKey(const std::string& privkey);
std::string DeriveAESKeyFromPassword(const std::string& password, CertParams& params);
std::string MixedCaseEncoding(const std::string& binaryAddress);

std::string keccak(const std::string &data);

std::string getAddressFromFile(const char* certContent);

#endif
