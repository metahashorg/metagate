#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <QString>
#include <vector>

#include <cryptopp/eccrypto.h>

class Wallet {
public:

    const static std::string PREFIX_ONE_KEY_MTH;

    const static std::string PREFIX_ONE_KEY_TMH;

public:

    static void createWallet(const QString &folder, const std::string &password, std::string &publicKey, std::string &addr);

    static QString makeFullWalletPath(const QString &folder, const std::string &addr);

    static std::vector<std::pair<QString, QString>> getAllWalletsInFolder(const QString &folder);

    static std::string getPrivateKey(const QString &folder, const std::string &addr, bool isCompact, bool isTMH);

    static void savePrivateKey(const QString &folder, const std::string &data, const std::string &password);

    static void checkAddress(const std::string &address);

public:

    Wallet(const QString &folder, const std::string &name, const std::string &password);

    std::string sign(const std::string &message, std::string &publicKey);

    static bool verify(const std::string &message, const std::string &signature, const std::string &publicKey);

    static std::string genTx(const std::string &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const std::string &data);

    void sign(const std::string &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const std::string &data, std::string &txHex, std::string &signature, std::string &publicKey);

    static std::string calcHash(const std::string &txHex);

    const QString& getFullPath() const {
        return fullPath;
    }

private:

    static std::string createAddress(const std::string &publicKeyBinary);

private:

    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;

    QString folder;
    std::string name;

    QString fullPath;
};

#endif // WALLET_H
