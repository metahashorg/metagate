#ifndef WALLET_H
#define WALLET_H

#include <string>
#include <QString>
#include <vector>

#include <cryptopp/eccrypto.h>

class Wallet {
public:

    static void createWallet(const QString &folder, const std::string &password, std::string &publicKey, std::string &addr);

    static QString makeFullWalletPath(const QString &folder, const std::string &addr);

    static std::vector<std::pair<QString, QString>> getAllWalletsInFolder(const QString &folder);

    /*
       Возвращает публичный ключ в base16
    */
    static std::string createRsaKey(const QString &folder, const std::string &addr, const std::string &password);

    static std::string decryptMessage(const QString &folder, const std::string &addr, const std::string &password, const std::string &encryptedMessageHex);

    static std::string encryptMessage(const std::string &publicKeyHex, const std::string &message);

public:

    Wallet(const QString &folder, const std::string &name, const std::string &password);

    std::string sign(const std::string &message, std::string &publicKey);

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
