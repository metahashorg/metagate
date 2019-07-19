#ifndef GETACTUALWALLETSEVENT_H
#define GETACTUALWALLETSEVENT_H

#include "qt_utilites/EventWatcher.h"
#include "qt_utilites/CallbackWrapper.h"

#include <QString>
#include <QObject>
#include <QThread>

#include <vector>

#include "WalletInfo.h"

namespace wallets {

class Wallets;

using GetActualWalletsCallback = CallbackWrapper<void(const QString &userName, const std::vector<WalletInfo> &walletAddresses)>;

class GetActualWalletsEvent: public QObject, public WatchedEvent {
    Q_OBJECT
public:

    GetActualWalletsEvent(QThread *currentThread, Wallets &wallets, const QString &expectedUserName, const WalletCurrency &currency, const GetActualWalletsCallback &callback);

protected:

    void callError(const TypedException &excetption) override;

public slots:

    void changedUserName(const QString &newUsername);

private:

    Wallets &wallets;

    QString expectedUserName;

    WalletCurrency currency;

    GetActualWalletsCallback callback;

};

} // namespace wallets

#endif // GETACTUALWALLETSEVENT_H
