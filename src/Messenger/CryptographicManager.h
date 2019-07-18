#ifndef CRYPTOGRAPHICMANAGER_H
#define CRYPTOGRAPHICMANAGER_H

#include <QObject>

#include "qt_utilites/TimerClass.h"
#include "qt_utilites/CallbackWrapper.h"

#include "Message.h"

class Wallet;
class WalletRsa;

struct TypedException;

namespace messenger {

class CryptographicManager : public QObject, public TimerClass {
    Q_OBJECT
public:

    explicit CryptographicManager(QObject *parent = nullptr);

    ~CryptographicManager() override;

    bool isSaveDecrypted() const {
        return isSaveDecrypted_;
    }

protected:

    void startMethod() override;

    void timerMethod() override;

    void finishMethod() override;

public:

    using DecryptMessagesCallback = CallbackWrapper<void(const std::vector<Message> &messages)>;

    using SignMessageCallback = CallbackWrapper<void(const QString &pubkey, const QString &sign)>;

    using SignMessagesCallback = CallbackWrapper<void(const QString &pubkey, const std::vector<QString> &sign)>;

    using SignTransactionCallback = CallbackWrapper<void(const QString &transaction, const QString &pubkey, const QString &sign)>;

    using GetPubkeyRsaCallback = CallbackWrapper<void(const QString &pubkeyRsa)>;

    using EncryptMessageCallback = CallbackWrapper<void(const QString &encryptedData)>;

    using UnlockWalletCallback = CallbackWrapper<void()>;

    using LockWalletCallback = CallbackWrapper<void()>;

    using RemainingTimeCallback = CallbackWrapper<void(const QString &address, const seconds &elapsed)>;

signals:

    void decryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback);

    void tryDecryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback);

    void signMessage(const QString &address, const QString &message, const SignMessageCallback &callback);

    void signMessages(const QString &address, const std::vector<QString> &messages, const SignMessagesCallback &callback);

    void signTransaction(const QString &address, const QString &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const QString &data, const SignTransactionCallback &callback);

    void getPubkeyRsa(const QString &address, const GetPubkeyRsaCallback &callback);

    void encryptDataRsa(const QString &dataHex, const QString &pubkeyDest, const EncryptMessageCallback &callback);

    void encryptDataPrivateKey(const QString &dataHex, const QString &address, const EncryptMessageCallback &callback);

    void unlockWallet(const QString &folder, bool isMhc, const QString &address, const QString &password, const QString &passwordRsa, const seconds &time_, const UnlockWalletCallback &callbackWrapper);

    void lockWallet(const LockWalletCallback &callback);

    void remainingTime(const RemainingTimeCallback &callback);

private slots:

    void onDecryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback);

    void onTryDecryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback);

    void onSignMessage(const QString &address, const QString &message, const SignMessageCallback &callback);

    void onSignMessages(const QString &address, const std::vector<QString> &messages, const SignMessagesCallback &callback);

    void onSignTransaction(const QString &address, const QString &toAddress, uint64_t value, uint64_t fee, uint64_t nonce, const QString &data, const SignTransactionCallback &callback);

    void onGetPubkeyRsa(const QString &address, const GetPubkeyRsaCallback &callback);

    void onEncryptDataRsa(const QString &dataHex, const QString &pubkeyDest, const EncryptMessageCallback &callback);

    void onEncryptDataPrivateKey(const QString &dataHex, const QString &address, const EncryptMessageCallback &callback);

    void onUnlockWallet(const QString &folder, bool isMhc, const QString &address, const QString &password, const QString &passwordRsa, const seconds &time_, const UnlockWalletCallback &callbackWrapper);

    void onLockWallet(const LockWalletCallback &callback);

    void onRemainingTime(const RemainingTimeCallback &callback);

private:

    Wallet& getWallet(const std::string &address) const;

    WalletRsa& getWalletRsa(const std::string &address) const;

    WalletRsa* getWalletRsaWithoutCheck(const std::string &address) const;

    void unlockWalletImpl(const QString &folder, bool isMhc, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_);

    void lockWalletImpl();

private:

    const bool isSaveDecrypted_;

    std::unique_ptr<Wallet> wallet;
    std::unique_ptr<WalletRsa> walletRsa;

    seconds time;
    time_point startTime;

};

}

#endif // CRYPTOGRAPHICMANAGER_H
