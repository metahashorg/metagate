#include "WalletInfo.h"

namespace wallets {

WalletInfo::WalletInfo() = default;

WalletInfo::WalletInfo(const QString &address, const QString &path, WalletInfo::Type type)
    : address(address)
    , path(path)
    , type(type)
{}

} // namespace wallets
