#include "CryptographicManager.h"

#include "check.h"
#include "SlotWrapper.h"
#include "TypedException.h"
#include "utils.h"

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
    CHECK(connect(this, &CryptographicManager::getPubkeyRsa, this, &CryptographicManager::onGetPubkeyRsa), "not connect onGetPubkeyRsa");
    CHECK(connect(this, &CryptographicManager::encryptDataRsa, this, &CryptographicManager::onEncryptDataRsa), "not connect onEncryptDataRsa");
    CHECK(connect(this, &CryptographicManager::encryptDataPrivateKey, this, &CryptographicManager::onEncryptDataPrivateKey), "not connect onSignAndEncryptDataRsa");
    CHECK(connect(this, &CryptographicManager::unlockWallet, this, &CryptographicManager::onUnlockWallet), "not connect onUnlockWallet");
    CHECK(connect(this, &CryptographicManager::lockWallet, this, &CryptographicManager::onLockWallet), "not connect onLockWallet");

    qRegisterMetaType<DecryptMessagesCallback>("DecryptMessagesCallback");
    qRegisterMetaType<std::vector<Message>>("std::vector<Message>");
    qRegisterMetaType<std::vector<QString>>("std::vector<QString>");
    qRegisterMetaType<SignMessageCallback>("SignMessageCallback");
    qRegisterMetaType<SignMessagesCallback>("SignMessagesCallback");
    qRegisterMetaType<GetPubkeyRsaCallback>("GetPubkeyRsaCallback");
    qRegisterMetaType<EncryptMessageCallback>("EncryptMessageCallback");
    qRegisterMetaType<UnlockWalletCallback>("UnlockWalletCallback");
    qRegisterMetaType<LockWalletCallback>("LockWalletCallback");
    qRegisterMetaType<std::string>("std::string");
    qRegisterMetaType<seconds>("seconds");

    moveToThread(&thread1);
}

Wallet& CryptographicManager::getWallet(const std::string &address) const {
    CHECK_TYPED(wallet != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet not unlock");
    CHECK_TYPED(wallet->getAddress() == address, TypeErrors::WALLET_OTHER, "Wallet address not coincide")
    return *wallet;
}

WalletRsa& CryptographicManager::getWalletRsa(const std::string &address) const {
    CHECK_TYPED(walletRsa != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet rsa not unlock");
    CHECK_TYPED(wallet != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet not unlock");
    CHECK_TYPED(wallet->getAddress() == address, TypeErrors::WALLET_OTHER, "Wallet address not coincide")
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

}
