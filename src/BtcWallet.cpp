#include "BtcWallet.h"

#include "btctx/wif.h"
#include "ethtx/utils2.h"
#include "btctx/btctx.h"

#include "check.h"
#include "utilites/utils.h"
#include "Log.h"

const std::string BtcWallet::PREFIX_ONE_KEY = "btc:";

const static std::string WIF_AND_ADDRESS_DELIMITER = " ";

static QString convertAddressToFileName(const std::string &address) {
    return QString::fromStdString(address.substr(0, address.size() - 3) + "---").toLower();
}

QString BtcWallet::getFullPath(const QString &folder, const std::string &address) {
    QString pathToFile = makePath(folder, QString::fromStdString(address).toLower());
    if (!QFile(pathToFile).exists()) {
        pathToFile = makePath(folder, convertAddressToFileName(address));
    }
    return pathToFile;
}

static std::pair<std::string, std::string> getWifAndAddress(const std::string &data, bool isNoEncrypted) {
    const size_t foundDelimiter = data.find(WIF_AND_ADDRESS_DELIMITER);
    std::string wif;
    std::string address;
    if (foundDelimiter == data.npos) {
        wif = data;
        if (isNoEncrypted) {
            bool tmp;
            address = ::getAddress(wif, tmp, false);
        }
    } else {
        wif = data.substr(0, foundDelimiter);
        address = data.substr(foundDelimiter + 1);
    }

    return std::make_pair(wif, address);
}

static std::pair<std::string, std::string> getWifAndAddress(const QString &folder, const std::string &addr, bool isNoEncrypted) {
    const QString pathToFile = BtcWallet::getFullPath(folder, addr);
    const std::string wifAndAddress = readFile(pathToFile);
    return getWifAndAddress(wifAndAddress, isNoEncrypted);
}

static std::string decryptWif(const std::string &wifEncrypted, const QString &password) {
    std::string wif = wifEncrypted;
    if (!password.isNull() && !password.isEmpty()) {
        if (wifEncrypted.substr(0, 2) == "6P") {
            wif = ::decryptWif(wifEncrypted, password.normalized(QString::NormalizationForm_C).toStdString());
        }
    } else {
        CHECK_TYPED(wifEncrypted.substr(0, 2) != "6P", TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encrypted wif " + wifEncrypted);
    }
    return wif;
}

std::pair<std::string, std::string> BtcWallet::genPrivateKey(const QString &folder, const QString &password) {
    const bool isCompressed = true;
    const bool isTestnet = false;
    std::string wif = CreateWIF(isTestnet, isCompressed);
    bool tmp;
    const std::string addressBase58 = ::getAddress(wif, tmp, isTestnet);
    CHECK(isCompressed == tmp, "ups");
    if (!password.isNull() && !password.isEmpty()) {
        wif = encryptWif(wif, password.normalized(QString::NormalizationForm_C).toStdString());
        CHECK_TYPED(wif.substr(0, 2) == "6P", TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encrypted wif " + wif);
    } else {
        CHECK_TYPED(wif.substr(0, 2) != "6P", TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encrypted wif " + wif);
    }

    const QString fileName = QDir(folder).filePath(convertAddressToFileName(addressBase58));
    const std::string fileData = wif + WIF_AND_ADDRESS_DELIMITER + addressBase58;
    BtcWallet checkWallet(fileData, password);
    CHECK_TYPED(!checkWallet.getAddress().empty(), TypeErrors::PRIVATE_KEY_ERROR, "dont check private key");
    writeToFile(fileName, fileData, true);

    return std::make_pair(addressBase58, wif);
}

BtcWallet::BtcWallet(const std::string &fileData, const QString &password) {
    const auto pair = getWifAndAddress(fileData, false);
    const std::string wifEncrypted = pair.first;
    address = pair.second;

    wif = decryptWif(wifEncrypted, password);

    if (address.empty()) {
        bool tmp;
        address = ::getAddress(wif, tmp, false);
    } else {
        bool tmp;
        const std::string calcAddress = ::getAddress(wif, tmp, false);
        CHECK_TYPED(calcAddress == address, TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encrypted wif: address " + address + " incorrect. Correct: " + calcAddress);
    }
}

BtcWallet::BtcWallet(const QString &folder, const std::string &address_, const QString &password)
    : BtcWallet(readFile(getFullPath(folder, address_)), password)
{}

BtcWallet::BtcWallet(const std::string &decryptedWif)
    : wif(decryptedWif)
{
    CHECK_TYPED(decryptedWif.substr(0, 2) != "6P", TypeErrors::PRIVATE_KEY_ERROR, "Incorrect encrypted wif");
}

const std::string& BtcWallet::getAddress() const {
    return address;
}

std::string BtcWallet::genTransaction(const std::vector<BtcInput> &inputs, uint64_t transferAmount, uint64_t fee, const std::string &receiveAddress, bool isTestnet) {
    checkAddressBase56(receiveAddress);

    std::vector<Input> inputs2;
    for (const BtcInput &input: inputs) {
        Input input2;
        input2.wif = wif;
        input2.outBalance = input.outBalance;
        input2.scriptPubkey = HexStringToDump(input.scriptPubkey);
        input2.spendoutnum = input.spendoutnum;
        input2.spendtxid = HexStringToDump(input.spendtxid);

        inputs2.push_back(input2);
    }

    const std::string tx = BuildBTCTransaction(inputs2, fee, transferAmount, receiveAddress, isTestnet);
    return DumpToHexString(tx);
}

static size_t calcSizeTransaction(const std::string& transaction) {
    return transaction.size() / 2;
}

/**
 * Пытается набрать как можно меньше элементов на заданную сумму
 * Может вернуть меньшую сумму, если элементов не достаточно.
 */
template<class Element, typename Value>
static std::vector<Element> greedyAlg(const std::vector<Element> &elements, const Value &allValue) {
    std::vector<Element> sortedVect(elements.begin(), elements.end());
    std::sort(sortedVect.begin(), sortedVect.end());
    std::vector<Element> result;
    Value currValue = 0;
    while (currValue < allValue && !sortedVect.empty()) {
        auto foundIter = std::lower_bound(sortedVect.begin(), sortedVect.end(), Element(allValue - currValue));
        if (foundIter == sortedVect.end()) {
            foundIter--;
        }
        currValue += foundIter->outBalance;
        result.emplace_back(*foundIter);
        sortedVect.erase(foundIter);
    }

    return result;
}

std::pair<std::string, std::set<std::string>> BtcWallet::encode(
    bool allMoney, const int64_t &value, const int64_t &fees,
    const std::string &toAddress,
    const std::vector<BtcInput> &utxos
) {
    LOG << "Utxos size " + std::to_string(utxos.size());

    std::vector<BtcInput> newUtxos;
    if (!allMoney) {
        const int64_t allValue = value + fees;
        newUtxos = greedyAlg(utxos, allValue);
        LOG << "Utxos size2 " + std::to_string(newUtxos.size());
    } else {
        newUtxos = utxos;
    }

    int64_t allUtxoValue = 0;
    for (const BtcInput &utxo: newUtxos) {
        allUtxoValue += utxo.outBalance;
    }

    CHECK_TYPED(allUtxoValue >= value + fees, TypeErrors::INCORRECT_USER_DATA, "Not enough money. Balance " + std::to_string(allUtxoValue) + ". Value to send " + std::to_string(value) + ". Fees " + std::to_string(fees));

    int64_t feesValue = fees;
    int64_t valueToSend;
    if (allMoney) {
        valueToSend = allUtxoValue - feesValue;
    } else {
        valueToSend = value;
    }

    CHECK_TYPED(valueToSend > 0, TypeErrors::INCORRECT_USER_DATA, "Not enough money. Balance " + std::to_string(allUtxoValue) + ". Value to send " + std::to_string(valueToSend) + ". Fees " + std::to_string(fees));
    CHECK_TYPED(valueToSend >= fees, TypeErrors::INCORRECT_VALUE_OR_FEE, "Value it should be large than fees. Value " + std::to_string(valueToSend) + ". Fees " + std::to_string(fees));

    const std::string encodedTransaction = genTransaction(newUtxos, valueToSend, feesValue, toAddress, false);

    std::set<std::string> usedUtxos;
    std::transform(newUtxos.begin(), newUtxos.end(), std::inserter(usedUtxos, usedUtxos.begin()), [](const BtcInput &input){return input.spendtxid;});

    return std::make_pair(encodedTransaction, usedUtxos);
}

std::vector<BtcInput> BtcWallet::reduceInputs(const std::vector<BtcInput> &inputs, const std::set<std::string> &usedTxs) {
    std::vector<BtcInput> result;
    std::copy_if(inputs.begin(), inputs.end(), std::back_inserter(result), [&usedTxs](const BtcInput &input) {
        return usedTxs.find(input.spendtxid) == usedTxs.end();
    });
    return result;
}

std::pair<std::string, std::set<std::string>> BtcWallet::buildTransaction(
    const std::vector<BtcInput> &utxos,
    size_t estimateComissionInSatoshi,
    const std::string &valueStr,
    const std::string &feesStr,
    const std::string &receiveAddress
) {
    bool allMoney = false;
    int64_t value = 0;
    if (valueStr == "all") {
        allMoney = true;
        value = 0;
    } else {
        CHECK_TYPED(isDecimal(valueStr), TypeErrors::INCORRECT_USER_DATA, "Not dec number value");
        allMoney = false;
        value = std::stoll(valueStr);
    }

    int64_t feesEstimate = 0;
    int64_t fees = 0;
    if (feesStr != "auto") {
        CHECK_TYPED(isDecimal(feesStr), TypeErrors::INCORRECT_USER_DATA, "Not dec number fees");
        fees = std::stoll(feesStr);
    } else {
        CHECK_TYPED(estimateComissionInSatoshi > 0, TypeErrors::INCORRECT_USER_DATA, "Uncnown estimate comission " + std::to_string(estimateComissionInSatoshi));
        feesEstimate = estimateComissionInSatoshi;
        LOG << "estimated fees1 " + std::to_string(feesEstimate);
    }
    int maxIterations = 10;
    std::string oldTransaction;
    std::set<std::string> oldUsedTransactions;
    while (true) {
        const size_t oldTransactionSize = calcSizeTransaction(oldTransaction);
        if (feesStr == "auto") {
            fees = (feesEstimate * oldTransactionSize) / 1024;
        }
        if (fees < (int64_t)oldTransactionSize + 30) {
            fees = oldTransactionSize + 30;
        }

        const auto tmpTransactionPair = encode(allMoney, value, fees, receiveAddress, utxos);
        const std::string &tmpTransaction = tmpTransactionPair.first;
        const std::set<std::string> &tmpUsedUtxos = tmpTransactionPair.second;
        if (tmpTransaction.empty()) {
            break;
        }

        oldTransaction = tmpTransaction;
        oldUsedTransactions = tmpUsedUtxos;

        if (std::abs((int)calcSizeTransaction(tmpTransaction) - (int)oldTransactionSize) <= 30) { // Проверяем, что размер транзакции не изменился после рассчета fees-а.
            break;
        }

        maxIterations--;
        if (maxIterations <= 0) {
            throwErr("I can not estimate fees");
        }
    }
    LOG << "estimated fees2 " + std::to_string(feesEstimate);

    const std::string &encodedTransaction = oldTransaction;
    const std::set<std::string> &usedTransactions = oldUsedTransactions;

    LOG << "transaction size " + std::to_string(calcSizeTransaction(encodedTransaction)) << " count used transactions " << usedTransactions.size();
    //LOGDEBUG << encodedTransaction;

    CHECK_TYPED(!encodedTransaction.empty(), TypeErrors::DONT_SIGN, "Not encode transactions");

    return std::make_pair(encodedTransaction, usedTransactions);
}

std::string BtcWallet::calcHashNotWitness(const std::string &txHex) {
    return toHex(calcHashTxNotWitness(fromHex(txHex)));
}

std::vector<std::pair<QString, QString>> BtcWallet::getAllWalletsInFolder(const QString &folder) {
    std::vector<std::pair<QString, QString>> result;

    const QDir dir(folder);
    const QStringList allFiles = dir.entryList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
    for (const QString &file: allFiles) {
        try {
            const std::string address = getWifAndAddress(folder, file.toStdString(), true).second;
            if (address.empty()) {
                continue;
            }
            if (!isAddressBase56(address)) {
                continue;
            }
            result.emplace_back(QString::fromStdString(address), getFullPath(folder, address));
        } catch (const TypedException &error) {
            LOG << "Error: " << file << " " << error.description;
        } catch (const Exception &e) {
            LOG << "Error: " << file << " " << e;
        } catch (...) {
            LOG << "Error: " << file << " " << " Unknown error";
        }
    }

    return result;
}

std::string BtcWallet::getOneKey(const QString &folder, const std::string &address) {
    const QString filePath = getFullPath(folder, address);
    return readFile(filePath);
}

void BtcWallet::savePrivateKey(const QString &folder, const std::string &data, const QString &password) {
    const auto pair = getWifAndAddress(data, true);
    const std::string &addressBase58 = pair.second;
    const std::string &encodedWif = pair.first;
    decryptWif(encodedWif, password);
    const QString pathToFile = QDir(folder).filePath(convertAddressToFileName(addressBase58));
    writeToFile(pathToFile, data, true);
}

void BtcWallet::checkAddress(const std::string &address) {
    checkAddressBase56(address);
}
