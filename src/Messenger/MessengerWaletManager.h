#ifndef MESSENGERWALETMANAGER_H
#define MESSENGERWALETMANAGER_H

#include <QObject>
#include <QTimer>

#include <memory>
#include <string>

#include "Wallet.h"
#include "WalletRsa.h"

#include "duration.h"

class MessengerWaletManager : public QObject {
    Q_OBJECT
public:
    explicit MessengerWaletManager(QObject *parent = nullptr);

public:

    Wallet& getWallet(const std::string &address) const;

    WalletRsa& getWalletRsa(const std::string &address) const;

    void unlockWallet(const QString &folder, const std::string &address, const std::string &password, const std::string &passwordRsa, const seconds &time_);

    void lockWallet();

private:

    std::unique_ptr<Wallet> wallet;
    std::unique_ptr<WalletRsa> walletRsa;

    seconds time;
    time_point startTime;

    QTimer timer;

signals:

public slots:

    void onResetWallets();

};

#endif // MESSENGERWALETMANAGER_H
