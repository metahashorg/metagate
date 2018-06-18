#ifndef ETHWALLET_H
#define ETHWALLET_H

#include <string>
#include <vector>

#include <QString>

class EthWallet {
public:

    EthWallet(
        const QString &folder,
        const std::string &address,
        std::string password
    );

    std::string SignTransaction(
        std::string nonce,
        std::string gasPrice,
        std::string gasLimit,
        std::string to,
        std::string value,
        std::string data
    );

    static void checkAddress(const std::string &address);

    static QString getFullPath(const QString &folder, const std::string &address);

    static std::string genPrivateKey(const QString &folder, const std::string &password);

    static std::vector<std::pair<QString, QString>> getAllWalletsInFolder(const QString &folder);

    static std::string makeErc20Data(const std::string &valueHex, const std::string &address);

    static std::string getOneKey(const QString &folder, const std::string &address);

    static void savePrivateKey(const QString &folder, const std::string &data, const std::string &password);

private:

    std::vector<uint8_t> rawprivkey;

};

#endif // ETHWALLET_H
