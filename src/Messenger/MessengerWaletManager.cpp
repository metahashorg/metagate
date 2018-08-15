#include "MessengerWaletManager.h"

#include "Wallet.h"
#include "WalletRsa.h"

#include "check.h"
#include "SlotWrapper.h"
#include "Log.h"

static std::thread::id threadId() {
    return std::this_thread::get_id();
}

MessengerWaletManager::MessengerWaletManager(QObject *parent)
    : QObject(parent)
    , threadId(::threadId())
{
    timer.setInterval(milliseconds(1s).count());
    CHECK(connect(&timer, SIGNAL(timeout()), this, SLOT(onResetWallets())), "not connect uploadEvent");
    timer.start();
}

Wallet& MessengerWaletManager::getWallet(const std::string &address) const {
    CHECK(threadId == ::threadId(), "Incorrect thread");
    CHECK_TYPED(wallet != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet not unlock");
    CHECK_TYPED(wallet->getAddress() == address, TypeErrors::WALLET_OTHER, "Wallet address not coincide")
    return *wallet;
}

WalletRsa& MessengerWaletManager::getWalletRsa(const std::string &address) const {
    CHECK(threadId == ::threadId(), "Incorrect thread");
    CHECK_TYPED(walletRsa != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet rsa not unlock");
    CHECK_TYPED(wallet != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet not unlock");
    CHECK_TYPED(wallet->getAddress() == address, TypeErrors::WALLET_OTHER, "Wallet address not coincide")
    return *walletRsa;
}

void MessengerWaletManager::lockWallet() {
    CHECK(threadId == ::threadId(), "Incorrect thread");
    wallet = nullptr;
    walletRsa = nullptr;
}

void MessengerWaletManager::unlockWallet(const QString &folder, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_) {
    CHECK(threadId == ::threadId(), "Incorrect thread");
    wallet = std::make_unique<Wallet>(folder, address, password);
    walletRsa = std::make_unique<WalletRsa>(folder, address);
    walletRsa->unlock(passwordRsa);

    time = time_;
    startTime = ::now();
}

void MessengerWaletManager::onResetWallets() {
BEGIN_SLOT_WRAPPER
    CHECK(threadId == ::threadId(), "Incorrect thread");
    LOG << "Try reset wallets";
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
