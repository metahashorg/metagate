#include "MessengerWaletManager.h"

#include "Wallet.h"
#include "WalletRsa.h"

#include "check.h"
#include "SlotWrapper.h"
#include "Log.h"

MessengerWaletManager::MessengerWaletManager(QObject *parent)
    : QObject(parent)
{
    timer.setInterval(milliseconds(1s).count());
    CHECK(connect(&timer, SIGNAL(timeout()), this, SLOT(uploadEvent())), "not connect uploadEvent");
    timer.start();
}

Wallet& MessengerWaletManager::getWallet(const std::string &address) const {
    CHECK_TYPED(wallet != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet not unlock");
    CHECK_TYPED(wallet->getAddress() == address, TypeErrors::WALLET_OTHER, "Wallet address not coincide")
    return *wallet;
}

WalletRsa& MessengerWaletManager::getWalletRsa(const std::string &address) const {
    CHECK_TYPED(walletRsa != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet rsa not unlock");
    CHECK_TYPED(wallet != nullptr, TypeErrors::WALLET_NOT_UNLOCK, "wallet not unlock");
    CHECK_TYPED(wallet->getAddress() == address, TypeErrors::WALLET_OTHER, "Wallet address not coincide")
    return *walletRsa;
}

void MessengerWaletManager::lockWallet() {
    wallet = nullptr;
    walletRsa = nullptr;
}

void MessengerWaletManager::unlockWallet(const QString &folder, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_) {
    wallet = std::make_unique<Wallet>(folder, address, password);
    walletRsa = std::make_unique<WalletRsa>(folder, address);
    walletRsa->unlock(passwordRsa);

    time = time_;
    startTime = ::now();
}

void MessengerWaletManager::onResetWallets() {
BEGIN_SLOT_WRAPPER
    LOG << "Reset wallets";
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
