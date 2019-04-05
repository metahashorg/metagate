#ifndef WALLETNAMESDBRES_H
#define WALLETNAMESDBRES_H

#include <QString>

namespace wallet_names {

static const QString databaseName = "wallet_names";
static const QString databaseFileName = "wallet_names.db";
static const int databaseVersion = 1;

static const QString createWalletsTable = "CREATE TABLE wallets ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "address VARCHAR(100) NOT NULL, "
                                                "name TEXT NOT NULL DEFAULT '' "
                                                ")";

static const QString createInfoTable = "CREATE TABLE info ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "wallet_id INTEGER NOT NULL, "
                                                "user TEXT NOT NULL DEFAULT '', "
                                                "device TEXT NOT NULL DEFAULT '', "
                                                "currency TEXT NOT NULL, "
                                                "FOREIGN KEY (wallet_id) REFERENCES wallets(id)"
                                                ")";

static const QString createWalletsUniqueIndex = "CREATE UNIQUE INDEX walletsUniqueIdx ON wallets ( "
                                                    "address ASC) ";

static const QString createInfoUniqueIndex = "CREATE UNIQUE INDEX usersUniqueIdx ON info ( "
                                                    "wallet_id ASC, user ASC, device ASC, currency ASC) ";

static const QString giveNameWalletPart1 = "INSERT OR IGNORE INTO wallets (address, name) "
                                           "VALUES (:address, :name)";

static const QString giveNameWalletPart2 = "UPDATE wallets SET name = :name WHERE address = :address";

static const QString selectAll = "SELECT address, name, user, device, currency FROM wallets "
                                 "LEFT JOIN info ON info.wallet_id == wallets.id ";

static const QString insertWalletInfo = "INSERT OR IGNORE INTO info (wallet_id, user, device, currency) "
                                           "VALUES (("
                                               "SELECT id FROM wallets WHERE address = :address"
                                           "), :user, :device, :currency)";

static const QString selectName = "SELECT name FROM wallets "
                                 "WHERE address = :address ";

static const QString selectInfo = "SELECT address, name, user, device, currency FROM wallets "
                                 "LEFT JOIN info ON info.wallet_id == wallets.id "
                                 "WHERE wallets.address == :address";

static const QString selectForCurrencyAndUser = "SELECT address, name, user, device, currency FROM wallets "
                                 "LEFT JOIN info ON info.wallet_id == wallets.id "
                                 "WHERE wallets.id IN (SELECT wallet_id FROM info "
                                 "WHERE user == :user AND currency == :currency)";

} // namespace wallet_names

#endif // WALLETNAMESDBRES_H
