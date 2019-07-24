#ifndef TRANSACTIONSDBRES_H
#define TRANSACTIONSDBRES_H

#include <QString>

namespace transactions {

static const QString databaseName = "payments";
static const QString databaseFileName = "payments.db";
static const int databaseVersion = 5;

static const QString createPaymentsTable = "CREATE TABLE payments ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "currency VARCHAR(100), "
                                                "txid TEXT, "
                                                "address TEXT, "
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
                                                "blockNumber INTEGER DEFAULT 0, "
                                                "ind INT8 NOT NULL DEFAULT 0, "
                                                "blockHash TEXT NOT NULL DEFAULT '', "
                                                "type INTEGER DEFAULT 0, "
                                                "intStatus INTEGER DEFAULT 0, "
                                                "status INT8 "
                                                ")";

static const QString createPaymentsUniqueIndex = "CREATE UNIQUE INDEX paymentsUniqueIdx ON payments ( "
                                                    "currency ASC, address ASC, txid ASC, blockNumber ASC, ind ASC ) ";

static const QString createBalanceTable = "CREATE TABLE balance ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "currency VARCHAR(100), "
                                                "address TEXT NOT NULL, "
                                                "received TEXT NOT NULL, "
                                                "spent TEXT NOT NULL, "
                                                "countReceived INT8 NOT NULL, "
                                                "countSpent INT8 NOT NULL, "
                                                "countTxs INT8 NOT NULL, "
                                                "currBlockNum INT8 NOT NULL, "
                                                "countDelegated INT8 NOT NULL, "
                                                "delegate TEXT NOT NULL, "
                                                "undelegate TEXT NOT NULL, "
                                                "delegated TEXT NOT NULL, "
                                                "undelegated TEXT NOT NULL, "
                                                "reserved TEXT NOT NULL, "
                                                "forged TEXT NOT NULL "
                                                ")";

static const QString createBalanceUniqueIndex = "CREATE UNIQUE INDEX balanceUniqueIdx ON balance ( "
                                                    "currency ASC, address ASC) ";

static const QString createPaymentsIndex1 = "CREATE INDEX paymentsIdx1 ON payments(address, currency, txid, blockNumber, ind)";
static const QString createPaymentsIndex2 = "CREATE INDEX paymentsIdx2 ON payments(address, currency, ts, txid)";
static const QString createPaymentsIndex3 = "CREATE INDEX paymentsIdx3 ON payments(currency, ts, txid)";
static const QString createPaymentsIndex4 = "CREATE INDEX paymentsIdx4 ON payments(address, currency, status, ts, txid)";
static const QString createPaymentsIndex5 = "CREATE INDEX paymentsIdx5 ON payments(address, currency, type, ts, txid)";
static const QString createPaymentsIndex6 = "CREATE INDEX paymentsIdx6 ON payments(address, currency, ufrom, uto, type, status, ts, txid)";
static const QString createPaymentsIndex7 = "CREATE INDEX paymentsIdx7 ON payments(address, currency, ufrom, type, status, ts, txid)";
static const QString createPaymentsIndex8 = "CREATE INDEX paymentsIdx8 ON payments(address, currency, blockNumber)";

static const QString createBalanceIndex1 = "CREATE INDEX balanceIdx1 ON balance(address, currency)";

static const QString createTrackedTable = "CREATE TABLE tracked ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "address TEXT, "
                                                "currency VARCHAR(100), "
                                                "name TEXT, "
                                                "type TEXT, "
                                                "tgroup TEXT "
                                                ")";

static const QString createTrackedUniqueIndex = "CREATE UNIQUE INDEX trackedUniqueIdx ON tracked ( "
                                                    "tgroup, address, currency) ";

static const QString deleteBalance = "DELETE FROM balance WHERE address = :address AND  currency = :currency ";

static const QString insertBalance = "INSERT OR IGNORE INTO balance (currency, address, received, spent, countReceived, countSpent, countTxs, currBlockNum, countDelegated, delegate, undelegate, delegated, undelegated, reserved, forged) "
                                        "VALUES (:currency, :address, :received, :spent, :countReceived, :countSpent, :countTxs, :currBlockNum, :countDelegated, :delegate, :undelegate, :delegated, :undelegated, :reserved, :forged)";

static const QString insertPayment = "INSERT OR IGNORE INTO payments (currency, txid, address, ind, ufrom, uto, value, ts, data, fee, nonce, isSetDelegate, isDelegate, delegateValue, delegateHash, status, type, blockNumber, blockHash, intStatus) "
                                        "VALUES (:currency, :txid, :address, :ind, :ufrom, :uto, :value, :ts, :data, :fee, :nonce, :isSetDelegate, :isDelegate, :delegateValue, :delegateHash, :status, :type, :blockNumber, :blockHash, :intStatus)";

static const QString selectBalance = "SELECT * FROM balance "
                                                    "WHERE address = :address AND  currency = :currency ";

static const QString selectPaymentsForDestFilter = "SELECT * FROM payments "
                                                    "WHERE address = :address AND  currency = :currency "
                                                    "%filter% "
                                                    "ORDER BY ts %1, txid %1 "
                                                    "LIMIT :count OFFSET :offset";

static const QString selectPaymentsForCurrency = "SELECT * FROM payments "
                                                    "WHERE currency = :currency "
                                                    "ORDER BY ts %1, txid %1 "
                                                    "LIMIT :count OFFSET :offset";

static const QString selectPaymentsForDestPending = "SELECT * FROM payments "
                                                        "WHERE address = :address AND  currency = :currency  "
                                                        "AND status = 1 "
                                                        "ORDER BY ts %1, txid %1";

static const QString selectLastTransaction = "SELECT * FROM payments "
                                                            "WHERE address = :address AND  currency = :currency "
                                                            "ORDER BY blockNumber DESC "
                                                            "LIMIT 1";

static const QString selectLastForgingTransaction = "SELECT * FROM payments "
                                                            "WHERE address = :address AND  currency = :currency "
                                                            "AND type = %1 "
                                                            "ORDER BY ts DESC "
                                                            "LIMIT 1";

static const QString updatePaymentForAddress = "UPDATE payments "
                                                    "SET ufrom = :ufrom, uto = :uto, "
                                                    "    value = :value, ts = :ts, data = :data, fee = :fee, nonce = :nonce, "
                                                    "    isSetDelegate = :isSetDelegate, isDelegate = :isDelegate, "
                                                    "    delegateValue = :delegateValue, delegateHash = :delegateHash, "
                                                    "    status = :status, type = :type, blockHash = :blockHash, intStatus = :intStatus "
                                                    "WHERE currency = :currency AND txid = :txid "
                                                    "    AND address = :address AND blockNumber = :blockNumber AND ind = :ind";

static const QString deletePaymentsForAddress = "DELETE FROM payments "
                                                "WHERE address = :address AND  currency = :currency";

static const QString selectPaymentsCountForAddress2 = "SELECT COUNT(DISTINCT txid || ',' || blockNumber || ',' || ind) AS count FROM payments "
                                                    "WHERE address = :address AND currency = :currency ";

static const QString insertTracked = "INSERT OR IGNORE INTO tracked (currency, address, name, type, tgroup) "
                                            "VALUES (:currency, :address, :name, :type, :tgroup)";

static const QString  selectTrackedForGroup = "SELECT currency, address, name, type FROM tracked "
                                                "WHERE tgroup = :tgroup "
                                                "ORDER BY address ASC";

static const QString removePaymentsForCurrencyQuery = "DELETE FROM payments %1";

static const QString removeTrackedForCurrencyQuery = "DELETE FROM tracked %1";

static const QString removeTrackedForGroupQuery = "DELETE FROM tracked WHERE tgroup = :tgroup AND currency = :currency";

static const QString removePaymentsCurrencyWhere = "WHERE currency = :currency";

};

#endif // TRANSACTIONSDBRES_H
