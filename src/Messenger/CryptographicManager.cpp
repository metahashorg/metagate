#include "CryptographicManager.h"

#include "Wallet.h"
#include "WalletRsa.h"

#include "check.h"
#include "SlotWrapper.h"
#include "TypedException.h"
#include "utils.h"
#include "QRegister.h"

SET_LOG_NAMESPACE("MSG");

namespace messenger {

CryptographicManager::CryptographicManager(QObject *parent)
    : TimerClass(1s, parent)
    , isSaveDecrypted_(false)
{
    CHECK(connect(this, &TimerClass::timerEvent, this, &CryptographicManager::onResetWallets), "not connect onTimerEvent");

    CHECK(connect(this, &CryptographicManager::decryptMessages, this, &CryptographicManager::onDecryptMessages), "not connect onDecryptMessages");
    CHECK(connect(this, &CryptographicManager::tryDecryptMessages, this, &CryptographicManager::onTryDecryptMessages), "not connect onTryDecryptMessages");
    CHECK(connect(this, &CryptographicManager::signMessage, this, &CryptographicManager::onSignMessage), "not connect onSignMessage");
    CHECK(connect(this, &CryptographicManager::signMessages, this, &CryptographicManager::onSignMessages), "not connect onSignMessages");
    CHECK(connect(this, &CryptographicManager::signTransaction, this, &CryptographicManager::onSignTransaction), "not connect onSignTransaction");
    CHECK(connect(this, &CryptographicManager::getPubkeyRsa, this, &CryptographicManager::onGetPubkeyRsa), "not connect onGetPubkeyRsa");
    CHECK(connect(this, &CryptographicManager::encryptDataRsa, this, &CryptographicManager::onEncryptDataRsa), "not connect onEncryptDataRsa");
    CHECK(connect(this, &CryptographicManager::encryptDataPrivateKey, this, &CryptographicManager::onEncryptDataPrivateKey), "not connect onSignAndEncryptDataRsa");
    CHECK(connect(this, &CryptographicManager::unlockWallet, this, &CryptographicManager::onUnlockWallet), "not connect onUnlockWallet");
    CHECK(connect(this, &CryptographicManager::lockWallet, this, &CryptographicManager::onLockWallet), "not connect onLockWallet");
    CHECK(connect(this, &CryptographicManager::remainingTime, this, &CryptographicManager::onRemainingTime), "not connect onRemainingTime");

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

    moveToThread(&thread1);
}

CryptographicManager::~CryptographicManager() = default;

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

void CryptographicManager::unlockWalletImpl(const QString &folder, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_) {
    wallet = std::make_unique<Wallet>(folder, address, password);
    walletRsa = std::make_unique<WalletRsa>(folder, address);
    walletRsa->unlock(passwordRsa);

    time = time_;
    startTime = ::now();
}

void CryptographicManager::onResetWallets() {
BEGIN_SLOT_WRAPPER
    LOG << PeriodicLog::make("r_wlt") << "Try reset wallets";
    if (wallet != nullptr || walletRsa != nullptr) {
        const time_point now = ::now();
        const milliseconds elapsedTime = std::chrono::duration_cast<milliseconds>(now - startTime);

        if (elapsedTime >= time) {
            LOG << "Reseted wallets";
            wallet = nullptr;
            walletRsa = nullptr;
        }
    }
END_SLOT_WRAPPER
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
    std::vector<Message> result;
    const TypedException exception = apiVrapper2([&, this] {
        result = decryptMsg(messages, getWalletRsaWithoutCheck(address.toStdString()), true);
    });

    callback.emitFunc(exception, result);
END_SLOT_WRAPPER
}

void CryptographicManager::onTryDecryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<Message> result;
    const TypedException exception = apiVrapper2([&, this] {
        result = decryptMsg(messages, getWalletRsaWithoutCheck(address.toStdString()), false);
    });

    callback.emitFunc(exception, result);
END_SLOT_WRAPPER
}

void CryptographicManager::onSignMessage(const QString &address, const QString &message, const SignMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString sign;
    QString pub;
    const TypedException exception = apiVrapper2([&, this] {
        std::string pubkey;
        sign = QString::fromStdString(getWallet(address.toStdString()).sign(message.toStdString(), pubkey));
        pub = QString::fromStdString(pubkey);
    });

    callback.emitFunc(exception, pub, sign);
END_SLOT_WRAPPER
}

void CryptographicManager::onSignTransaction(const QString &address, const QString &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const QString &data, const SignTransactionCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString sign;
    QString pub;
    QString tx;
    const TypedException exception = apiVrapper2([&, this] {
        std::string pubkey;
        std::string transaction;
        std::string signature;
        getWallet(address.toStdString()).sign(toAddress.toStdString(), value, fee, nonce, data.toStdString(), transaction, signature, pubkey, true);
        sign = QString::fromStdString(signature);
        pub = QString::fromStdString(pubkey);
        tx = QString::fromStdString(transaction);
    });

    callback.emitFunc(exception, tx, pub, sign);
END_SLOT_WRAPPER
}

void CryptographicManager::onSignMessages(const QString &address, const std::vector<QString> &messages, const SignMessagesCallback &callback) {
BEGIN_SLOT_WRAPPER
    std::vector<QString> sign;
    QString pub;
    const TypedException exception = apiVrapper2([&, this] {
        std::string pubkey;
        for (const QString &message: messages) {
            pubkey.clear();
            sign.emplace_back(QString::fromStdString(getWallet(address.toStdString()).sign(message.toStdString(), pubkey)));
        }
        pub = QString::fromStdString(pubkey);
    });

    callback.emitFunc(exception, pub, sign);
END_SLOT_WRAPPER
}

void CryptographicManager::onGetPubkeyRsa(const QString &address, const GetPubkeyRsaCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString pubkey;
    const TypedException exception = apiVrapper2([&, this] {
        pubkey = QString::fromStdString(getWalletRsa(address.toStdString()).getPublikKey());
    });

    callback.emitFunc(exception, pubkey);
END_SLOT_WRAPPER
}

void CryptographicManager::onEncryptDataRsa(const QString &dataHex, const QString &pubkeyDest, const EncryptMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString encryptedData;
    const TypedException exception = apiVrapper2([&] {
        const std::string data = fromHex(dataHex.toStdString());
        const WalletRsa walletRsa = WalletRsa::fromPublicKey(pubkeyDest.toStdString());
        encryptedData = QString::fromStdString(walletRsa.encrypt(data));
    });

    callback.emitFunc(exception, encryptedData);
END_SLOT_WRAPPER
}

void CryptographicManager::onEncryptDataPrivateKey(const QString &dataHex, const QString &address, const EncryptMessageCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString encryptedData;
    const TypedException exception = apiVrapper2([&] {
        const std::string data = fromHex(dataHex.toStdString());
        encryptedData = QString::fromStdString(getWalletRsa(address.toStdString()).encrypt(data));
    });

    callback.emitFunc(exception, encryptedData);
END_SLOT_WRAPPER
}

void CryptographicManager::onUnlockWallet(const QString &folder, const QString &address, const QString &password, const QString &passwordRsa, const seconds &time_, const UnlockWalletCallback &callbackWrapper) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&] {
        unlockWalletImpl(folder, address.toStdString(), password.toStdString(), passwordRsa.toStdString(), time_);
    });

    callbackWrapper.emitFunc(exception);
END_SLOT_WRAPPER
}

void CryptographicManager::onLockWallet(const LockWalletCallback &callback) {
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&] {
        lockWalletImpl();
    });

    callback.emitFunc(exception);
END_SLOT_WRAPPER
}

void CryptographicManager::onRemainingTime(const RemainingTimeCallback &callback) {
BEGIN_SLOT_WRAPPER
    QString address;
    seconds remaining(0);
    const TypedException exception = apiVrapper2([&] {
        if (wallet == nullptr || walletRsa == nullptr) {
            return;
        }
        address = QString::fromStdString(wallet->getAddress());
        const time_point now = ::now();
        const milliseconds elapsedTime = std::chrono::duration_cast<milliseconds>(now - startTime);
        remaining = std::chrono::duration_cast<seconds>(time - elapsedTime);
        if (remaining < seconds(0)) {
            remaining = seconds(0);
        }
    });

    callback.emitFunc(exception, address, remaining);
END_SLOT_WRAPPER
}

}
