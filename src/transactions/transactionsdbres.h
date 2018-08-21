#ifndef TRANSACTIONSDBRES_H
#define TRANSACTIONSDBRES_H

#include <QString>

namespace transactions {

static const QString databaseFileName = "payments.db";
static const QString databaseVersion = "1";

static const QString createPaymentsTable = "CREATE TABLE payments ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "currency VARCHAR(100), "
                                                "txid TEXT, "
                                                "isInput BOOLEAN, "
                                                "ufrom TEXT, "
                                                "uto TEXT, "
                                                "value TEXT, "
                                                "ts INT8, "
                                                "data TEXT, "
                                                "fee INTEGER, "
                                                "nonce INTEGER"
                                                ")";


static const QString insertPayment = "INSERT INTO payments (currency, txid, isInput, ufrom, uto, value, ts, data, fee, nonce) "
                                        "VALUES (:currency, :txid, :isInput, :ufrom, :uto, :value, :ts, :data, :fee, :nonce)";

static const QString selectPaymentsForDest = "SELECT * FROM payments "
                                                    "WHERE ufrom = :ufrom AND uto = :uto "
                                                    "ORDER BY ts %1";
};

#endif // TRANSACTIONSDBRES_H
