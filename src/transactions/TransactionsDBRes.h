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
                                                "fee TEXT, "
                                                "nonce INTEGER, "
                                                "isSetDelegate BOOLEAN, "
                                                "isDelegate BOOLEAN, "
                                                "delegateValue TEXT, "
                                                "delegateHash TEXT, "
                                                "status INT8 "
                                                ")";

static const QString createPaymentsUniqueIndex = "CREATE UNIQUE INDEX paymentsUniqueIdx ON payments ( "
                                                    "currency ASC, address ASC, txid ASC, isInput ASC ) ";

static const QString createPaymentsIndex1 = "CREATE INDEX paymentsIdx1 ON payments(address, currency, isInput, isDelegate, isSetDelegate)";
static const QString createPaymentsIndex2 = "CREATE INDEX paymentsIdx2 ON payments(address, currency, ts, txid)";
static const QString createPaymentsIndex3 = "CREATE INDEX paymentsIdx3 ON payments(currency, ts, txid)";

static const QString createTrackedTable = "CREATE TABLE tracked ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "address TEXT, "
                                                "currency VARCHAR(100), "
                                                "name TEXT, "
                                                "type TEXT, "
                                                "tgroup TEXT "
                                                ")";

static const QString createTrackedUniqueIndex = "CREATE UNIQUE INDEX trackedUniqueIdx ON tracked ( "
                                                    "tgroup, address, currency, name, type ) ";

static const QString insertPayment = "INSERT OR IGNORE INTO payments (currency, txid, address, isInput, ufrom, uto, value, ts, data, fee, nonce, isSetDelegate, isDelegate, delegateValue, delegateHash, status) "
                                        "VALUES (:currency, :txid, :address, :isInput, :ufrom, :uto, :value, :ts, :data, :fee, :nonce, :isSetDelegate, :isDelegate, :delegateValue, :delegateHash, :status)";

static const QString selectPaymentsForDest = "SELECT id, currency, txid, address, isInput, ufrom, uto, "
                                                    "value, ts, data, fee, nonce, isSetDelegate, isDelegate, delegateValue, delegateHash, status FROM payments "
                                                    "WHERE address = :address AND  currency = :currency "
                                                    "ORDER BY ts %1, txid %1 "
                                                    "LIMIT :count OFFSET :offset";

static const QString selectPaymentsForCurrency = "SELECT id, currency, txid, address, isInput, ufrom, uto, "
                                                    "value, ts, data, fee, nonce, isSetDelegate, isDelegate, delegateValue, delegateHash, status FROM payments "
                                                    "WHERE currency = :currency "
                                                    "ORDER BY ts %1, txid %1 "
                                                    "LIMIT :count OFFSET :offset";

static const QString selectPaymentsForDestPending = "SELECT id, currency, txid, address, isInput, ufrom, uto, "
                                                        "value, ts, data, fee, nonce, isSetDelegate, isDelegate, delegateValue, delegateHash, status FROM payments "
                                                        "WHERE address = :address AND  currency = :currency  "
                                                        "AND status = 1 "
                                                        "ORDER BY ts %1, txid %1";

static const QString selectLastPaymentIsSetDelegate = "SELECT id, currency, txid, address, isInput, ufrom, uto, "
                                                            "value, ts, data, fee, nonce, isSetDelegate, isDelegate, delegateValue, delegateHash, status FROM payments "
                                                            "WHERE address = :address AND  currency = :currency "
                                                            "AND ufrom = :ufrom AND uto = :uto AND isInput = :isInput AND isDelegate = :isDelegate "
                                                            "AND isSetDelegate = 1 "
                                                            "ORDER BY ts DESC "
                                                            "LIMIT 1";

static const QString updatePaymentForAddress = "UPDATE payments "
                                                    "SET ufrom = :ufrom, uto = :uto, "
                                                    "    value = :value, ts = :ts, data = :data, fee = :fee, nonce = :nonce, "
                                                    "    isSetDelegate = :isSetDelegate, isDelegate = :isDelegate, "
                                                    "    delegateValue = :delegateValue, delegateHash = :delegateHash, "
                                                    "    status = :status "
                                                    "WHERE currency = :currency AND txid = :txid "
                                                    "    AND address = :address AND isInput = :isInput";

static const QString deletePaymentsForAddress = "DELETE FROM payments "
                                                "WHERE address = :address AND  currency = :currency";

static const QString selectInPaymentsValuesForAddress = "SELECT value, fee FROM payments "
                                                         "WHERE address = :address AND currency = :currency "
                                                         "AND isInput = 1";

static const QString selectOutPaymentsValuesForAddress = "SELECT value FROM payments "
                                                          "WHERE address = :address AND currency = :currency "
                                                          "AND isInput = 0";

static const QString selectIsSetDelegatePaymentsCountForAddress = "SELECT COUNT(*) AS count FROM payments "
                                                                        "WHERE address = :address AND currency = :currency "
                                                                        "AND isSetDelegate = 1 AND status = :status";

static const QString selectIsSetDelegatePaymentsValuesForAddress = "SELECT delegateValue FROM payments "
                                                                        "WHERE address = :address AND currency = :currency "
                                                                        "AND isInput = :isInput AND isDelegate = :isDelegate "
                                                                        "AND isSetDelegate = 1 AND status = :status";

static const QString selectPaymentsCountForAddress = "SELECT COUNT(*) AS count FROM payments "
                                                    "WHERE address = :address AND currency = :currency "
                                                    "AND isInput = :input";

static const QString insertTracked = "INSERT OR IGNORE INTO tracked (currency, address, name, type, tgroup) "
                                            "VALUES (:currency, :address, :name, :type, :tgroup)";

static const QString  selectTrackedForGroup = "SELECT currency, address, name, type FROM tracked "
                                                "WHERE tgroup = :tgroup "
                                                "ORDER BY address ASC";

static const QString removePaymentsForCurrencyQuery = "DELETE FROM payments "
                                                "WHERE currency = :currency";

static const QString removeTrackedForCurrencyQuery = "DELETE FROM tracked "
                                                "WHERE currency = :currency";

};

#endif // TRANSACTIONSDBRES_H
