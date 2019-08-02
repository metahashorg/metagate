#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <QString>
#include <vector>

#include <cryptopp/eccrypto.h>

#include "WalletInfo.h"

class Wallet {
public:

    const static std::string PREFIX_ONE_KEY_MTH;

    const static std::string PREFIX_ONE_KEY_TMH;

public:

    static QString chooseSubfolder(bool isMhc);

    static void createWallet(const QString &folder, bool isMhc, const std::string &password, std::string &publicKey, std::string &addr);

    static void createWalletFromRaw(const QString &folder, bool isMhc, const std::string &rawPrivateHex, const std::string &password, std::string &publicKey, std::string &addr);

    static void createWalletWatch(const QString &folder, bool isMhc, const std::string &addr);

    static void removeWalletWatch(const QString &folder, bool isMhc, const std::string &addr);

    static bool isWalletExists(const QString &folder, bool isMhc, const std::string &addr);

    static QString makeFullWalletPath(const QString &folder, bool isMhc, const std::string &addr);

    static QString makeFullWalletWatchPath(const QString &folder, bool isMhc, const std::string &addr);

    static std::vector<std::pair<QString, QString>> getAllWalletsInFolder(const QString &folder, bool isMhc, bool all = false);

    static std::vector<wallets::WalletInfo> getAllWalletsInfoInFolder(const QString &folder, bool isMhc);

    static std::string getPrivateKey(const QString &folder, bool isMhc, const std::string &addr, bool isCompact);

    static void savePrivateKey(const QString &folder, bool isMhc, const std::string &data, const std::string &password);

    static void saveWalletWatch(const QString &folder, bool isMhc, const std::string &addr);

    static void checkAddress(const std::string &address, bool isCheckHash=true);

    static std::string createV8Address(const std::string &address, int nonce);

    static bool isCorrectFilenameWallet(const QString &filePath);

public:

    Wallet(const QString &folder, bool isMhc, const std::string &name, const std::string &password);

    Wallet(const QString &folder, bool isMhc, const std::string &name);

    std::string sign(const std::string &message, std::string &publicKey) const;

    static bool verify(const std::string &message, const std::string &signature, const std::string &publicKey);

    static std::string genTx(const std::string &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const std::string &dataHex, bool isCheckHash);

    void sign(const std::string &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const std::string &data, std::string &txHex, std::string &signature, std::string &publicKey, bool isCheckHash=true);

    std::string getNotProtectedKeyHex() const;

    static std::string genDataDelegateHex(bool isDelegate, uint64_t value);

    static std::string calcHash(const std::string &txHex, const std::string &signHex, const std::string &pubkeyHex);

    const QString& getFullPath() const {
        return fullPath;
    }

    const std::string& getAddress() const {
        return name;
    }

public:

    static std::string createAddress(const std::string &publicKeyBinary);

private:

    static void savePrivateKey(const QString &folder, bool isMhc, const CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey &privKey, const std::string &password, std::string &publicKey, std::string &addr);

private:
    wallets::WalletInfo::Type type;
    CryptoPP::ECDSA<CryptoPP::ECP, CryptoPP::SHA256>::PrivateKey privateKey;

    std::string name;

    QString fullPath;
};

#endif // WALLET_H
