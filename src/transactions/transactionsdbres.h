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
                                                "address TEXT, "
                                                "isInput BOOLEAN, "
                                                "ufrom TEXT, "
                                                "uto TEXT, "
                                                "value TEXT, "
                                                "ts INT8, "
                                                "data TEXT, "
                                                "fee INTEGER, "
                                                "nonce INTEGER"
                                                ")";

static const QString createPaymentsSortingIndex = "CREATE INDEX paymentsSortingIdx ON payments(ts ASC, txid ASC)";
static const QString createPaymentsUniqueIndex = "CREATE UNIQUE INDEX paymentsUniqueIdx ON payments ( "
                                                    "currency, txid, address, isInput ) ";

static const QString createTrackedTable = "CREATE TABLE tracked ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "address TEXT, "
                                                "currency VARCHAR(100), "
                                                "name TEXT, "
                                                "type TEXT, "
                                                "tgroup TEXT "
                                                ")";

static const QString createTrackedUniqueIndex = "CREATE UNIQUE INDEX trackedUniqueIdx ON tracked ( "
                                                    "currency, address, name, type, tgroup ) ";

static const QString insertPayment = "INSERT OR IGNORE INTO payments (currency, txid, address, isInput, ufrom, uto, value, ts, data, fee, nonce) "
                                        "VALUES (:currency, :txid, :address, :isInput, :ufrom, :uto, :value, :ts, :data, :fee, :nonce)";

static const QString selectPaymentsForDest = "SELECT id, currency, txid, address, isInput, ufrom, uto, "
                                                    "value, ts, data, fee, nonce FROM payments "
                                                    "WHERE address = :address AND  currency = :currency "
                                                    "ORDER BY ts %1, txid %1 "
                                                    "LIMIT :count OFFSET :offset";

static const QString selectPaymentsForCurrency = "SELECT id, currency, txid, address, isInput, ufrom, uto, "
                                                    "value, ts, data, fee, nonce FROM payments "
                                                    "WHERE currency = :currency "
                                                    "ORDER BY ts %1, txid %1 "
                                                    "LIMIT :count OFFSET :offset";

static const QString deletePaymentsForAddress = "DELETE FROM payments "
                                                "WHERE address = :address AND  currency = :currency";

static const QString selectInPaymentsValuesForAddress = "SELECT value, fee FROM payments "
                                                         "WHERE address = :address AND  currency = :currency "
                                                         "AND isInput = 1";

static const QString selectOutPaymentsValuesForAddress = "SELECT value FROM payments "
                                                          "WHERE address = :address AND  currency = :currency "
                                                          "AND isInput = 0";

static const QString selectPaymentsCountForAddress = "SELECT COUNT(*) AS count FROM payments "
                                                    "WHERE address = :address AND  currency = :currency "
                                                    "AND isInput = :input";

static const QString insertTracked = "INSERT OR IGNORE INTO tracked (currency, address, name, type, tgroup) "
                                            "VALUES (:currency, :address, :name, :type, :tgroup)";

static const QString  selectTrackedForGroup = "SELECT currency, address, name, type FROM tracked "
                                                "WHERE tgroup = :tgroup "
                                                "ORDER BY address ASC";
};

#endif // TRANSACTIONSDBRES_H
