#ifndef DBRES_H
#define DBRES_H

#include <QString>

static const QString sqliteSettings = "PRAGMA foreign_keys=on";

static const QString dropTable = "DROP TABLE IF EXISTS %1";


static const QString createPaymentsTable = "CREATE TABLE payments ("
                                           "id VARCHAR(100) PRIMARY KEY,"
                                           "trans VARCHAR(100),"
                                           "from_account VARCHAR(100),"
                                           "to_account VARCHAR(100),"
                                           "amount VARCHAR(9),"
                                           "value VARCHAR(9),"
                                           "block INT,"
                                           "is_input BOOLEAN,"
                                           "ts INT,"
                                           "confirmations VARCHAR(9))";

static const QString preparePaymentsInsert = "INSERT OR REPLACE INTO payments "
        "(id, trans, from_account, to_account, amount, value, block, is_input, ts, confirmations)"
        "VALUES (:id, :trans, :from_account, :to_account, :amount, :value, :block, :is_input, :ts, :confirmations)";


static const QString selectPayments = "SELECT id, trans, from_account, to_account, "
                                    "amount, value, block, is_input, ts, confirmations "
                                    "FROM payments";




static const QString createSettingsTable = "CREATE TABLE settings ( "
                                           "key VARCHAR(256), "
                                           "value TEXT "
                                           ")";

static const QString createMsgUsersTable = "CREATE TABLE msg_users ( "
                                           "id INTEGER PRIMARY KEY NOT NULL, "
                                           "username VARCHAR(100), "
                                           "publickey VARCHAR(200), "
                                           "signatures VARCHAR(200) "
                                           ")";

static const QString createMsgContactsTable = "CREATE TABLE msg_contacts ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "username VARCHAR(100), "
                                                "publickey VARCHAR(200) "
                                                ")";

static const QString createMsgMessagesTable = "CREATE TABLE msg_messages ( "
                                           "id INTEGER PRIMARY KEY NOT NULL, "
                                           "userid  INTEGER NOT NULL, "
                                           "contactid  INTEGER NOT NULL, "
                                           "morder INT8, "
                                           "dt INT8, "
                                           "text TEXT, "
                                           "isIncoming BOOLEAN, "
                                           "canDecrypted BOOLEAN, "
                                           "isConfirmed BOOLEAN, "
                                           "hash VARCHAR(100), "
                                           "fee INT8, "
                                           "FOREIGN KEY (userid) REFERENCES msg_users(id), "
                                           "FOREIGN KEY (contactid) REFERENCES msg_contacts(id) "
                                           ")";

static const QString createMsgLastReadMessageTable = "CREATE TABLE msg_lastreadmessage ( "
                                                        "id INTEGER PRIMARY KEY NOT NULL, "
                                                        "userid  INTEGER NOT NULL, "
                                                        "contactid  INTEGER NOT NULL, "
                                                        "lastcounter INT8, "
                                                        "FOREIGN KEY (userid) REFERENCES msg_users(id), "
                                                        "FOREIGN KEY (contactid) REFERENCES msg_contacts(id) "
                                                        ")";

static const QString selectMsgUsersForName = "SELECT id FROM msg_users WHERE username = :username";
static const QString insertMsgUsers = "INSERT INTO msg_users (username) VALUES (:username)";

static const QString selectMsgContactsForName = "SELECT id FROM msg_contacts WHERE username = :username";
static const QString insertMsgContacts = "INSERT INTO msg_contacts (username) VALUES (:username)";

static const QString insertMsgMessages = "INSERT INTO msg_messages "
                                            "(userid, contactid, morder, dt, text, isIncoming, canDecrypted, isConfirmed, hash, fee) VALUES "
                                            "(:userid, :contactid, :order, :dt, :text, :isIncoming, :canDecrypted, :isConfirmed, :hash, :fee)";

static const QString selectMsgMaxCounter = "SELECT IFNULL(MAX(m.morder), -1) AS max "
                                           "FROM msg_messages m "
                                           "INNER JOIN msg_users u ON u.id = m.userid "
                                           "WHERE u.username = :user";

static const QString selectMsgMaxConfirmedCounter = "SELECT IFNULL(MAX(m.morder), -1) AS max "
                                                    "FROM msg_messages m "
                                                    "INNER JOIN msg_users u ON u.id = m.userid "
                                                    "WHERE m.isConfirmed = 1 "
                                                    "AND u.username = :user";

static const QString selectMsgMessagesForUser = "SELECT u.username AS user, du.username AS duser, m.isIncoming, m.text, "
                                                "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                "FROM msg_messages m "
                                                "INNER JOIN msg_users u ON u.id = m.userid "
                                                "INNER JOIN msg_contacts du ON du.id = m.contactid "
                                                "WHERE m.morder >= :ob AND m.morder <= :oe "
                                                "AND u.username = :user "
                                                "ORDER BY m.morder";

static const QString selectMsgMessagesForUserAndDest = "SELECT u.username AS user, du.username AS duser, m.isIncoming, m.text, "
                                                       "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                       "FROM msg_messages m "
                                                       "INNER JOIN msg_users u ON u.id = m.userid "
                                                       "INNER JOIN msg_contacts du ON du.id = m.contactid "
                                                       "WHERE m.morder >= :ob AND m.morder <= :oe "
                                                       "AND u.username = :user AND du.username = :duser "
                                                       "ORDER BY m.morder";

static const QString selectMsgMessagesForUserAndDestNum = "SELECT u.username AS user, du.username AS duser, m.isIncoming, m.text, "
                                                          "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                          "FROM msg_messages m "
                                                          "INNER JOIN msg_users u ON u.id = m.userid "
                                                          "INNER JOIN msg_contacts du ON du.id = m.contactid "
                                                          "WHERE m.morder <= :oe "
                                                          "AND u.username = :user AND du.username = :duser "
                                                          "ORDER BY m.morder DESC "
                                                          "LIMIT :num";

static const QString selectMsgUsersList = "SELECT username FROM msg_users ORDER BY id";

static const QString selectMsgUserPublicKey = "SELECT publickey FROM msg_users WHERE username = :user";
static const QString updateMsgUserPublicKey = "UPDATE msg_users SET publickey = :publickey WHERE username = :user";

static const QString selectMsgUserSignatures = "SELECT signatures FROM msg_users WHERE username = :user";
static const QString updateMsgUserSignatures = "UPDATE msg_users SET signatures = :signatures WHERE username = :user";

static const QString selectMsgContactsPublicKey = "SELECT publickey FROM msg_contacts WHERE username = :user";
static const QString updateMsgContactsPublicKey = "UPDATE msg_contacts SET publickey = :publickey WHERE username = :user";

static const QString selectMsgCountMessagesForUserAndDest = "SELECT COUNT(*) "
                                                       "FROM msg_messages m "
                                                       "INNER JOIN msg_users u ON u.id = m.userid "
                                                       "INNER JOIN msg_contacts du ON du.id = m.contactid "
                                                       "WHERE m.morder >= :ob "
                                                       "AND u.username = :user AND du.username = :duser";


static const QString selectCountNotConfirmedMessagesWithHash = "SELECT COUNT(*) "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid "
                                                        "WHERE isConfirmed = 0 "
                                                        "AND hash = :hash "
                                                        "AND u.username = :user";

static const QString selectCountMessagesWithCounter = "SELECT COUNT(*) "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid "
                                                        "WHERE m.order = :counter "
                                                        "AND u.username = :user ";


static const QString selectFirstNotConfirmedMessage = "SELECT m.id "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid "
                                                        "WHERE m.isConfirmed = 0 "
                                                        "AND u.username = :user "
                                                        "ORDER BY m.morder "
                                                        "LIMIT 1";

static const QString selectFirstNotConfirmedMessageWithHash = "SELECT m.id "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid "
                                                        "WHERE m.isConfirmed = 0 "
                                                        "AND m.hash = :hash "
                                                        "AND u.username = :user "
                                                        "ORDER BY m.morder "
                                                        "LIMIT 1";


static const QString updateMessageQuery = "UPDATE msg_messages "
                                        "SET isConfirmed = :isConfirmed, morder = :counter "
                                        "WHERE id = :id";

static const QString selectLastReadCounterForUserContact = "SELECT l.lastcounter FROM msg_lastreadmessage l "
                                                            "INNER JOIN msg_users u ON u.id = l.userid "
                                                            "INNER JOIN msg_contacts c ON c.id = l.contactid "
                                                            "WHERE u.username = :user AND c.username = :contact";

static const QString updateLastReadCounterForUserContact = "UPDATE msg_lastreadmessage "
                                                            "SET lastcounter = :counter "
                                                            "WHERE id = (SELECT l.id FROM msg_lastreadmessage l "
                                                            "INNER JOIN msg_users u ON u.id = l.userid "
                                                            "INNER JOIN msg_contacts c ON c.id = l.contactid "
                                                            "WHERE u.username = :user AND c.username = :contact)";

static const QString selectLastReadMessageCount = "SELECT COUNT(*) FROM msg_lastreadmessage "
                                                    "WHERE userid = :userid AND contactid = :contactid";
static const QString insertLastReadMessageRecord = "INSERT INTO msg_lastreadmessage "
                                                    "(lastcounter, userid, contactid) VALUES "
                                                    "(0, :userid, :contactid)";
static const QString selectLastReadCountersForUser = "SELECT c.username, l.lastcounter "
                                                        "FROM msg_lastreadmessage l "
                                                        "INNER JOIN msg_users u ON u.id = l.userid "
                                                        "INNER JOIN msg_contacts c ON c.id = l.contactid "
                                                        "WHERE u.username = :user";

#endif // DBRES_H
