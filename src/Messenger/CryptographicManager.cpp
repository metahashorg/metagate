#include "CryptographicManager.h"

#include "Wallet.h"
#include "WalletRsa.h"

#include "check.h"
#include "qt_utilites/SlotWrapper.h"
#include "TypedException.h"
#include "utilites/utils.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/ManagerWrapperImpl.h"

SET_LOG_NAMESPACE("MSG");

namespace messenger {

CryptographicManager::CryptographicManager(QObject *parent)
    : TimerClass(1s, parent)
    , isSaveDecrypted_(false)
{
    Q_CONNECT(this, &CryptographicManager::decryptMessages, this, &CryptographicManager::onDecryptMessages);
    Q_CONNECT(this, &CryptographicManager::tryDecryptMessages, this, &CryptographicManager::onTryDecryptMessages);
    Q_CONNECT(this, &CryptographicManager::signMessage, this, &CryptographicManager::onSignMessage);
    Q_CONNECT(this, &CryptographicManager::signMessages, this, &CryptographicManager::onSignMessages);
    Q_CONNECT(this, &CryptographicManager::signTransaction, this, &CryptographicManager::onSignTransaction);
    Q_CONNECT(this, &CryptographicManager::getPubkeyRsa, this, &CryptographicManager::onGetPubkeyRsa);
    Q_CONNECT(this, &CryptographicManager::encryptDataRsa, this, &CryptographicManager::onEncryptDataRsa);
    Q_CONNECT(this, &CryptographicManager::encryptDataPrivateKey, this, &CryptographicManager::onEncryptDataPrivateKey);
    Q_CONNECT(this, &CryptographicManager::unlockWallet, this, &CryptographicManager::onUnlockWallet);
    Q_CONNECT(this, &CryptographicManager::lockWallet, this, &CryptographicManager::onLockWallet);
    Q_CONNECT(this, &CryptographicManager::remainingTime, this, &CryptographicManager::onRemainingTime);

    Q_REG(DecryptMessagesCallback, "DecryptMessagesCallback");
    Q_REG(std::vector<Message>, "std::vector<Message>");
    Q_REG2(std::vector<QString>, "std::vector<QString>", false);
    Q_REG(SignMessageCallback, "SignMessageCallback");
    Q_REG(SignMessagesCallback, "SignMessagesCallback");
    Q_REG(SignTransactionCallback, "SignTransactionCallback");
    Q_REG(GetPubkeyRsaCallback, "GetPubkeyRsaCallback");
    Q_REG(EncryptMessageCallback, "EncryptMessageCallback");
    Q_REG(UnlockWalletCallback, "UnlockWalletCallback");
    Q_REG(LockWalletCallback, "LockWalletCallback");
    Q_REG(RemainingTimeCallback, "RemainingTimeCallback");
    Q_REG2(std::string, "std::string", false);
    Q_REG2(seconds, "seconds", false);
    Q_REG2(uint64_t, "uint64_t", false);

    moveToThread(TimerClass::getThread());
}

CryptographicManager::~CryptographicManager() {
    TimerClass::exit();
}

void CryptographicManager::startMethod() {
    // empty
}

void CryptographicManager::finishMethod() {
    // empty
}

Wallet& CryptographicManager::getWallet(const std::string &address) const {
    CHECK_TYPED(wallet != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet not unlock " + address);
    CHECK_TYPED(wallet->getAddress() == address, TypeErrors::WALLET_OTHER, "Other wallet unlocked: " + wallet->getAddress() + ". " + address);
    return *wallet;
}

WalletRsa& CryptographicManager::getWalletRsa(const std::string &address) const {
    CHECK_TYPED(walletRsa != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet rsa not unlock: " + address);
    CHECK_TYPED(wallet != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet not unlock " + address);
    CHECK_TYPED(wallet->getAddress() == address, TypeErrors::WALLET_OTHER, "Other wallet unlocked: " + wallet->getAddress() + ". " + address);
    return *walletRsa;
}

WalletRsa* CryptographicManager::getWalletRsaWithoutCheck(const std::string &address) const {
    if (walletRsa == nullptr) {
        return nullptr;
    }
    if (wallet->getAddress() != address) {
        return nullptr;
    }
    return walletRsa.get();
}

void CryptographicManager::lockWalletImpl() {
    wallet = nullptr;
    walletRsa = nullptr;
}

void CryptographicManager::unlockWalletImpl(const QString &folder, bool isMhc, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_) {
    wallet = std::make_unique<Wallet>(folder, isMhc, address, password);
    walletRsa = std::make_unique<WalletRsa>(folder, isMhc, address);
    walletRsa->unlock(passwordRsa);

    time = time_;
    startTime = ::now();
}

void CryptographicManager::timerMethod() {
    if (wallet != nullptr || walletRsa != nullptr) {
        const time_point now = ::now();
        const milliseconds elapsedTime = std::chrono::duration_cast<milliseconds>(now - startTime);

        if (elapsedTime >= time && (wallet != nullptr || walletRsa != nullptr)) {
            LOG << "Reseted wallets";
            wallet = nullptr;
            walletRsa = nullptr;
        }
    }
}

static std::vector<Message> decryptMsg(const std::vector<Message> &messages, const WalletRsa *walletRsa, bool isThrow) {
    std::vector<Message> result;
    result.reserve(messages.size());
    std::transform(messages.begin(), messages.end(), std::back_inserter(result), [walletRsa, isThrow](const Message &message) {
        if (message.isDecrypted) {
            return message;
        }
        Message result = message;
        const bool isEncrypted = !result.isChannel;
        if (!isEncrypted) {
            result.decryptedDataHex = result.dataHex;
            result.isDecrypted = true;
        } else {
            if (result.isCanDecrypted) {
                if (walletRsa == nullptr) {
                    if (isThrow) {
                        throwErrTyped(TypeErrors::WALLET_NOT_UNLOCK, "Wallet rsa not unlock");
                    } else {
                        return result;
                    }
                }
                CHECK_TYPED(walletRsa != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "Wallet rsa not unlock");
                const std::string decryptedData = toHex(walletRsa->decryptMessage(result.dataHex.toStdString()));
                result.decryptedDataHex = QString::fromStdString(decryptedData);
                result.isDecrypted = true;
            }
        }
        return result;
    });
    return result;
}

void CryptographicManager::onDecryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return decryptMsg(messages, getWalletRsaWithoutCheck(address.toStdString()), true);
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onTryDecryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return decryptMsg(messages, getWalletRsaWithoutCheck(address.toStdString()), false);
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onSignMessage(const QString &address, const QString &message, const SignMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        std::string pubkey;
        const QString sign = QString::fromStdString(getWallet(address.toStdString()).sign(message.toStdString(), pubkey));
        const QString pub = QString::fromStdString(pubkey);
        return std::make_tuple(pub, sign);
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onSignTransaction(const QString &address, const QString &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const QString &data, const SignTransactionCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        std::string pubkey;
        std::string transaction;
        std::string signature;
        getWallet(address.toStdString()).sign(toAddress.toStdString(), value, fee, nonce, data.toStdString(), transaction, signature, pubkey, true);
        const QString sign = QString::fromStdString(signature);
        const QString pub = QString::fromStdString(pubkey);
        const QString tx = QString::fromStdString(transaction);
        return std::make_tuple(tx, pub, sign);
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onSignMessages(const QString &address, const std::vector<QString> &messages, const SignMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        std::string pubkey;
        std::vector<QString> sign;
        for (const QString &message: messages) {
            pubkey.clear();
            sign.emplace_back(QString::fromStdString(getWallet(address.toStdString()).sign(message.toStdString(), pubkey)));
        }
        const QString pub = QString::fromStdString(pubkey);
        return std::make_tuple(pub, sign);
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onGetPubkeyRsa(const QString &address, const GetPubkeyRsaCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&, this] {
        return QString::fromStdString(getWalletRsa(address.toStdString()).getPublikKey());
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onEncryptDataRsa(const QString &dataHex, const QString &pubkeyDest, const EncryptMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&] {
        const std::string data = fromHex(dataHex.toStdString());
        const WalletRsa walletRsa = WalletRsa::fromPublicKey(pubkeyDest.toStdString());
        return QString::fromStdString(walletRsa.encrypt(data));
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onEncryptDataPrivateKey(const QString &dataHex, const QString &address, const EncryptMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&] {
        const std::string data = fromHex(dataHex.toStdString());
        return QString::fromStdString(getWalletRsa(address.toStdString()).encrypt(data));
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onUnlockWallet(const QString &folder, bool isMhc, const QString &address, const QString &password, const QString &passwordRsa, const seconds &time_, const UnlockWalletCallback &callbackWrapper) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&] {
        unlockWalletImpl(folder, isMhc, address.toStdString(), password.toStdString(), passwordRsa.toStdString(), time_);
    }, callbackWrapper);
END_SLOT_WRAPPER
}

void CryptographicManager::onLockWallet(const LockWalletCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&] {
        lockWalletImpl();
    }, callback);
END_SLOT_WRAPPER
}

void CryptographicManager::onRemainingTime(const RemainingTimeCallback &callback) {
BEGIN_SLOT_WRAPPER
    runAndEmitCallback([&]() ->std::tuple<QString, seconds> {
        if (wallet == nullptr || walletRsa == nullptr) {
            return std::make_tuple("", 0s);
        }
        const QString address = QString::fromStdString(wallet->getAddress());
        const time_point now = ::now();
        const milliseconds elapsedTime = std::chrono::duration_cast<milliseconds>(now - startTime);
        seconds remaining = std::chrono::duration_cast<seconds>(time - elapsedTime);
        if (remaining < seconds(0)) {
            remaining = seconds(0);
        }
        return std::make_tuple(address, remaining);
    }, callback, "", 0s);
END_SLOT_WRAPPER
}

}
