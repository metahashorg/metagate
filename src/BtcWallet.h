#ifndef BTCWALLET_H
#define BTCWALLET_H

#include <string>
#include <vector>

#include <QString>

struct BtcInput {
    std::string spendtxid;
    uint32_t spendoutnum;
    std::string scriptPubkey;
    uint64_t outBalance;

    bool operator< (const BtcInput &second) const {
        return this->outBalance < second.outBalance;
    }

    BtcInput(uint64_t outBalance)
        : outBalance(outBalance)
    {}

    BtcInput() = default;

};

class BtcWallet {
public:

    const static std::string PREFIX_ONE_KEY;

public:

    static QString getFullPath(const QString &folder, const std::string &address);

    static std::pair<std::string, std::string> genPrivateKey(const QString &folder, const QString &password);

    BtcWallet(const QString &folder, const std::string &address, const QString &password);

    BtcWallet(const std::string &decryptedWif);

    std::string genTransaction(const std::vector<BtcInput> &inputs, uint64_t transferAmount, uint64_t fee, const std::string &receiveAddress, bool isTestnet);

    std::string buildTransaction(
        const std::vector<BtcInput> &utxos,
        size_t estimateComissionInSatoshi,
        const std::string &valueStr,
        const std::string &feesStr,
        const std::string &receiveAddress
    );

    static std::vector<std::pair<QString, QString>> getAllWalletsInFolder(const QString &folder);

    const std::string& getAddress() const;

    static std::string getOneKey(const QString &folder, const std::string &address);

    static void savePrivateKey(const QString &folder, const std::string &data, const QString &password);

    static void checkAddress(const std::string &address);

private:

    std::string encode(
        bool allMoney, const int64_t &value, const int64_t &fees,
        const std::string &toAddress,
        const std::vector<BtcInput> &utxos
    );

    std::string wif;

    std::string address;
};

#endif // BTCWALLET_H
