#include "GetActualWalletsEvent.h"

#include "Log.h"
#include "check.h"
#include "TypedException.h"

#include "Wallets.h"

SET_LOG_NAMESPACE("WLTS");

namespace wallets {

GetActualWalletsEvent::GetActualWalletsEvent(QThread *currentThread, Wallets &wallets, const QString &expectedUserName, const WalletCurrency &currency, const GetActualWalletsCallback &callback)
    : wallets(wallets)
    , expectedUserName(expectedUserName)
    , currency(currency)
    , callback(callback)
{
    moveToThread(currentThread);
}

void GetActualWalletsEvent::callError(const TypedException &excetption) {
    callback.emitException(excetption);
}

void GetActualWalletsEvent::changedUserName(const QString &newUsername) {
    if (newUsername != expectedUserName) {
        return;
    }
    emit wallets.getListWallets(currency, callback);
    callback.reset();
    removeLater();
}

} // namespace wallets
