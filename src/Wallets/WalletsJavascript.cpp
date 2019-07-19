#include "WalletsJavascript.h"

#include "Log.h"
#include "check.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/WrapperJavascriptImpl.h"

#include "Wallets.h"

SET_LOG_NAMESPACE("WLTS");

namespace wallets {

WalletsJavascript::WalletsJavascript(Wallets &wallets, QObject *parent)
    : WrapperJavascript(false, LOG_FILE)
    , wallets(wallets)
{

}

} // namespace wallets
