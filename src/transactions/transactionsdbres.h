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

static const QString createPaymentsSortingIndex = "CREATE INDEX paymentsSortingIdx ON payments(ts ASC, txid ASC)";

static const QString createTrackedTable = "CREATE TABLE tracked ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "ufrom TEXT, "
                                                "uto TEXT, "
                                                "currency VARCHAR(100), "
                                                "type TEXT, "
                                                "tgroup TEXT "
                                                ")";


static const QString insertPayment = "INSERT INTO payments (currency, txid, isInput, ufrom, uto, value, ts, data, fee, nonce) "
                                        "VALUES (:currency, :txid, :isInput, :ufrom, :uto, :value, :ts, :data, :fee, :nonce)";

static const QString selectPaymentsForDest = "SELECT id, currency, txid, isInput, ufrom, uto, "
                                                    "value, ts, data, fee, nonce FROM payments "
                                                    "WHERE ufrom = :ufrom AND uto = :uto AND  currency = :currency "
                                                    "ORDER BY ts %1, txid %1 "
                                                    "LIMIT :count OFFSET :offset";

static const QString selectPaymentsForCurrency = "SELECT id, currency, txid, isInput, ufrom, uto, "
                                                    "value, ts, data, fee, nonce FROM payments "
                                                    "WHERE ufrom = :ufrom AND currency = :currency "
                                                    "ORDER BY ts %1, txid %1 "
                                                    "LIMIT :count OFFSET :offset";

static const QString deletePaymentsForDest = "DELETE FROM payments "
                                                "WHERE ufrom = :ufrom AND uto = :uto AND  currency = :currency";

static const QString selectInPaymentsValuesForDest = "SELECT value FROM payments "
                                                         "WHERE ufrom = :ufrom AND uto = :uto AND  currency = :currency "
                                                         "AND isInput = 1";

static const QString selectOutPaymentsValuesForDest = "SELECT value, fee FROM payments "
                                                          "WHERE ufrom = :ufrom AND uto = :uto AND  currency = :currency "
                                                          "AND isInput = 0";

static const QString selectPaymentsCountForDest = "SELECT COUNT(*) AS count FROM payments "
                                                    "WHERE ufrom = :ufrom AND uto = :uto AND  currency = :currency "
                                                    "AND isInput = :input";

static const QString insertTracked = "INSERT INTO tracked (currency, ufrom, uto, type, tgroup) "
                                            "VALUES (:currency, :ufrom, :uto, :type, :tgroup)";

};

#endif // TRANSACTIONSDBRES_H
