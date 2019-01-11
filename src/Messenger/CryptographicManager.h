#ifndef CRYPTOGRAPHICMANAGER_H
#define CRYPTOGRAPHICMANAGER_H

#include <QObject>

#include "TimerClass.h"

#include "Message.h"

#include "Wallet.h"
#include "WalletRsa.h"

class TypedException;

namespace messenger {

class CryptographicManager : public TimerClass {
    Q_OBJECT
public:

    explicit CryptographicManager(QObject *parent = nullptr);

    bool isSaveDecrypted() const {
        return isSaveDecrypted_;
    }

public:

    using SignalFunc = std::function<void(const std::function<void()> &callback)>;

    using DecryptMessagesCallback = std::function<void(const std::vector<Message> &messages, const TypedException &exception)>;

    using SignMessageCallback = std::function<void(const QString &pubkey, const QString &sign, const TypedException &exception)>;

    using SignMessagesCallback = std::function<void(const QString &pubkey, const std::vector<QString> &sign, const TypedException &exception)>;

    using GetPubkeyRsaCallback = std::function<void(const QString &pubkeyRsa, const TypedException &exception)>;

    using EncryptMessageCallback = std::function<void(const QString &encryptedData, const TypedException &exception)>;

    using UnlockWalletCallback = std::function<void(const TypedException &exception)>;

    using LockWalletCallback = std::function<void(const TypedException &exception)>;

signals:

    void decryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback, const SignalFunc &signalFunc);

    void signMessage(const QString &address, const QString &message, const SignMessageCallback &callback, const SignalFunc &signalFunc);

    void signMessages(const QString &address, const std::vector<QString> &messages, const SignMessagesCallback &callback, const SignalFunc &signalFunc);

    void getPubkeyRsa(const QString &address, const GetPubkeyRsaCallback &callback, const SignalFunc &signalFunc);

    void encryptDataRsa(const QString &dataHex, const QString &pubkeyDest, const EncryptMessageCallback &callback, const SignalFunc &signalFunc);

    void encryptDataPrivateKey(const QString &dataHex, const QString &address, const EncryptMessageCallback &callback, const SignalFunc &signalFunc);

    void unlockWallet(const QString &folder, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_, const UnlockWalletCallback &callback, const SignalFunc &signalFunc);

    void lockWallet(const LockWalletCallback &callback, const SignalFunc &signalFunc);

private slots:

    void onDecryptMessages(const std::vector<Message> &messages, const QString &address, const DecryptMessagesCallback &callback, const SignalFunc &signalFunc);

    void onSignMessage(const QString &address, const QString &message, const SignMessageCallback &callback, const SignalFunc &signalFunc);

    void onSignMessages(const QString &address, const std::vector<QString> &messages, const SignMessagesCallback &callback, const SignalFunc &signalFunc);

    void onGetPubkeyRsa(const QString &address, const GetPubkeyRsaCallback &callback, const SignalFunc &signalFunc);

    void onEncryptDataRsa(const QString &dataHex, const QString &pubkeyDest, const EncryptMessageCallback &callback, const SignalFunc &signalFunc);

    void onEncryptDataPrivateKey(const QString &dataHex, const QString &address, const EncryptMessageCallback &callback, const SignalFunc &signalFunc);

    void onUnlockWallet(const QString &folder, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_, const UnlockWalletCallback &callback, const SignalFunc &signalFunc);

    void onLockWallet(const LockWalletCallback &callback, const SignalFunc &signalFunc);

private slots:

    void onResetWallets();

private:

    Wallet& getWallet(const std::string &address) const;

    WalletRsa& getWalletRsa(const std::string &address) const;

    void unlockWalletImpl(const QString &folder, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_);

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
