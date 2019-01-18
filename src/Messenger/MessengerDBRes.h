#ifndef MESSENGERDBRES_H
#define MESSENGERDBRES_H

#include <QString>

namespace messenger {

static const QString databaseName = "messenger";
static const QString databaseFileName = "messenger.db";
static const int databaseVersion = 1;

static const QString createMsgUsersTable = "CREATE TABLE users ( "
                                           "id INTEGER PRIMARY KEY NOT NULL, "
                                           "username VARCHAR(100) UNIQUE, "
                                           "publickey VARCHAR(200), "
                                           "signatures VARCHAR(200) "
                                           ")";

static const QString createUsersSortingIndex = "CREATE INDEX usersSortingIdx ON users(username)";

static const QString createMsgContactsTable = "CREATE TABLE contacts ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "username VARCHAR(100) UNIQUE, "
                                                "publickey VARCHAR(200) "
                                                ")";

static const QString createContactsSortingIndex = "CREATE INDEX contactsSortingIdx ON contacts(username)";

static const QString createMsgChannelsTable = "CREATE TABLE channels ( "
                                                        "id INTEGER PRIMARY KEY NOT NULL, "
                                                        "userid  INTEGER NOT NULL, "
                                                        "channel TEXT, "
                                                        "shaName TEXT, "
                                                        "isAdmin BOOLEAN, "
                                                        "adminName VARCHAR(200), "
                                                        "isBanned BOOLEAN, "
                                                        "isWriter BOOLEAN, "
                                                        "isVisited BOOLEAN, "
                                                        "FOREIGN KEY (userid) REFERENCES users(id) "
                                                        ")";


static const QString createMsgMessagesTable = "CREATE TABLE messages ( "
                                           "id INTEGER PRIMARY KEY NOT NULL, "
                                           "userid  INTEGER NOT NULL, "
                                           "contactid  INTEGER, "
                                           "morder INT8, "
                                           "dt INT8, "
                                           "text TEXT, "
                                           "decryptedText TEXT, "
                                           "isDecrypted BOOLEAN, "
                                           "isIncoming BOOLEAN, "
                                           "canDecrypted BOOLEAN, "
                                           "isConfirmed BOOLEAN, "
                                           "hash VARCHAR(100), "
                                           "fee INT8, "
                                           "channelid  INTEGER, "
                                           "FOREIGN KEY (userid) REFERENCES users(id), "
                                           "FOREIGN KEY (contactid) REFERENCES contacts(id), "
                                           "FOREIGN KEY (channelid) REFERENCES channels(id) "
                                           ")";

static const QString createMsgMessageUniqueIndex = "CREATE UNIQUE INDEX messagesUniqueIdx ON messages ( "
                                                    "userid, contactid, morder, dt, text, decryptedText, isDecrypted, isIncoming, canDecrypted, "
                                                    "isConfirmed, hash, fee)";

static const QString createMsgMessageCounterIndex = "CREATE INDEX messagesCounterIdx ON messages(morder)";

static const QString createMsgLastReadMessageTable = "CREATE TABLE lastreadmessage ( "
                                                        "id INTEGER PRIMARY KEY NOT NULL, "
                                                        "userid  INTEGER NOT NULL, "
                                                        "contactid  INTEGER, "
                                                        "channelid  INTEGER, "
                                                        "lastcounter INT8, "
                                                        "FOREIGN KEY (userid) REFERENCES users(id), "
                                                        "FOREIGN KEY (contactid) REFERENCES contacts(id), "
                                                        "FOREIGN KEY (channelid) REFERENCES channels(id) "
                                                        ")";

static const QString createLastReadMessageUniqueIndex1 = "CREATE UNIQUE INDEX lastreadmessageUniqueIdx1 ON lastreadmessage ( "
                                                          "userid, contactid)";

static const QString createLastReadMessageUniqueIndex2 = "CREATE UNIQUE INDEX lastreadmessageUniqueIdx2 ON lastreadmessage ( "
                                                          "userid, channelid)";

static const QString selectMsgUsersForName = "SELECT id FROM users WHERE username = :username";
static const QString insertMsgUsers = "INSERT INTO users (username) VALUES (:username)";

static const QString selectMsgContactsForName = "SELECT id FROM contacts WHERE username = :username";
static const QString insertMsgContacts = "INSERT INTO contacts (username) VALUES (:username)";

static const QString insertMsgMessages = "INSERT OR IGNORE INTO messages "
                                            "(userid, contactid, morder, dt, text, decryptedText, isDecrypted, isIncoming, canDecrypted, isConfirmed, hash, fee, channelid) VALUES "
                                            "(:userid, :contactid, :order, :dt, :text, :decryptedText, :isDecrypted, :isIncoming, :canDecrypted, :isConfirmed, :hash, :fee, :channelid)";

static const QString selectMsgMaxCounter = "SELECT IFNULL(MAX(m.morder), -1) AS max "
                                           "FROM messages m "
                                           "INNER JOIN users u ON u.id = m.userid %1 "
                                           "WHERE u.username = :user %2";

static const QString selectMsgMaxConfirmedCounter = "SELECT IFNULL(MAX(m.morder), -1) AS max "
                                                    "FROM messages m "
                                                    "INNER JOIN users u ON u.id = m.userid "
                                                    "WHERE m.isConfirmed = 1 "
                                                    "AND u.username = :user";

static QString selectMsgMessagesForUser = "SELECT u.username AS user, du.username AS dest, m.isIncoming, m.text, m.decryptedText, m.isDecrypted, "
                                                "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                "FROM messages m "
                                                "INNER JOIN users u ON u.id = m.userid "
                                                "INNER JOIN contacts du ON du.id = m.contactid "
                                                "WHERE m.morder >= :ob AND m.morder <= :oe "
                                                "AND u.username = :user "
                                                "ORDER BY m.morder";

static const QString selectMsgMessagesForUserAndDest = "SELECT u.username AS user, c.username AS dest, m.isIncoming, m.text, m.decryptedText, m.isDecrypted, "
                                                       "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                       "FROM messages m "
                                                       "INNER JOIN users u ON u.id = m.userid "
                                                       "INNER JOIN contacts c ON c.id = m.contactid "
                                                       "WHERE m.morder >= :ob AND m.morder <= :oe "
                                                       "AND u.username = :user AND c.username = :duser "
                                                       "ORDER BY m.morder";

static const QString selectMsgMessagesForUserAndChannel = "SELECT u.username AS user, c.shaName AS dest, m.isIncoming, m.text, m.decryptedText, m.isDecrypted, "
                                                             "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                             "FROM messages m "
                                                             "INNER JOIN users u ON u.id = m.userid "
                                                             "INNER JOIN channels c ON c.id = m.channelid "
                                                             "WHERE m.morder >= :ob AND m.morder <= :oe "
                                                             "AND u.username = :user AND c.shaName = :shaName "
                                                             "ORDER BY m.morder";

static const QString selectMsgMessagesForUserAndDestNum = "SELECT u.username AS user, c.username AS dest, m.isIncoming, m.text, m.decryptedText, m.isDecrypted, "
                                                          "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                          "FROM messages m "
                                                          "INNER JOIN users u ON u.id = m.userid "
                                                          "INNER JOIN contacts c ON c.id = m.contactid "
                                                          "WHERE m.morder <= :oe "
                                                          "AND u.username = :user AND c.username = :duser "
                                                          "ORDER BY m.morder DESC "
                                                          "LIMIT :num";

static const QString selectMsgMessagesForUserAndChannelNum = "SELECT u.username AS user, c.shaName AS dest, m.isIncoming, m.text, m.decryptedText, m.isDecrypted, "
                                                          "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                          "FROM messages m "
                                                          "INNER JOIN users u ON u.id = m.userid "
                                                          "INNER JOIN channels c ON c.id = m.channelid "
                                                          "WHERE m.morder <= :oe "
                                                          "AND u.username = :user AND c.shaName = :shaName "
                                                          "ORDER BY m.morder DESC "
                                                          "LIMIT :num";

static const QString selectMsgUsersList = "SELECT username FROM users ORDER BY id";

static const QString selectMsgUserPublicKey = "SELECT publickey FROM users WHERE username = :user";
static const QString updateMsgUserPublicKey = "UPDATE users SET publickey = :publickey WHERE username = :user";

static const QString selectMsgUserSignatures = "SELECT signatures FROM users WHERE username = :user";
static const QString updateMsgUserSignatures = "UPDATE users SET signatures = :signatures WHERE username = :user";

static const QString selectMsgContactsPublicKey = "SELECT publickey FROM contacts WHERE username = :user";
static const QString updateMsgContactsPublicKey = "UPDATE contacts SET publickey = :publickey WHERE username = :user";

static const QString selectMsgCountMessagesForUserAndDest = "SELECT COUNT(*) AS count "
                                                       "FROM messages m "
                                                       "INNER JOIN users u ON u.id = m.userid "
                                                       "INNER JOIN contacts du ON du.id = m.contactid "
                                                       "WHERE m.morder >= :ob "
                                                       "AND u.username = :user AND du.username = :duser";


static const QString selectCountNotConfirmedMessagesWithHash = "SELECT (COUNT(*) > 0) AS res "
                                                        "FROM messages m "
                                                        "INNER JOIN users u ON u.id = m.userid "
                                                        "WHERE m.isConfirmed = 0 "
                                                        "AND m.hash = :hash "
                                                        "AND u.username = :user";

static const QString selectCountMessagesWithCounter = "SELECT (COUNT(*) > 0) AS res "
                                                        "FROM messages m "
                                                        "INNER JOIN users u ON u.id = m.userid %1 "
                                                        "WHERE m.morder = :counter "
                                                        "AND u.username = :user %2 ";


static const QString selectFirstNotConfirmedMessage = "SELECT m.id, m.morder "
                                                        "FROM messages m "
                                                        "INNER JOIN users u ON u.id = m.userid "
                                                        "WHERE m.isConfirmed = 0 "
                                                        "AND u.username = :user "
                                                        "ORDER BY m.morder "
                                                        "LIMIT 1";

static const QString selectFirstNotConfirmedMessageWithHash = "SELECT m.id, m.morder "
                                                        "FROM messages m "
                                                        "INNER JOIN users u ON u.id = m.userid %1 "
                                                        "WHERE m.isConfirmed = 0 "
                                                        "AND m.hash = :hash "
                                                        "AND u.username = :user %2 "
                                                        "ORDER BY m.morder "
                                                        "LIMIT 1";

static const QString selectFirstMessageWithHash = "SELECT m.id, m.morder "
                                                        "FROM messages m "
                                                        "INNER JOIN users u ON u.id = m.userid %1 "
                                                        "WHERE m.hash = :hash "
                                                        "AND u.username = :user %2 "
                                                        "ORDER BY m.morder "
                                                        "LIMIT 1";

static const QString updateMessageQuery = "UPDATE messages "
                                        "SET isConfirmed = :isConfirmed, morder = :counter "
                                        "WHERE id = :id";

static const QString selectLastReadCounterForUserContact = "SELECT l.lastcounter FROM lastreadmessage l "
                                                            "INNER JOIN users u ON u.id = l.userid "
                                                            "INNER JOIN contacts c ON c.id = l.contactid "
                                                            "WHERE u.username = :user AND c.username = :contact";

static const QString selectLastReadCounterForUserChannel = "SELECT l.lastcounter FROM lastreadmessage l "
                                                            "INNER JOIN users u ON u.id = l.userid "
                                                            "INNER JOIN channels c ON c.id = l.channelid "
                                                            "WHERE u.username = :user AND c.shaName = :shaName";

static const QString updateLastReadCounterForUserContact = "UPDATE lastreadmessage "
                                                            "SET lastcounter = :counter "
                                                            "WHERE id = (SELECT l.id FROM lastreadmessage l "
                                                            "INNER JOIN users u ON u.id = l.userid "
                                                            "INNER JOIN contacts c ON c.id = l.contactid "
                                                            "WHERE u.username = :user AND c.username = :contact)";

static const QString updateLastReadCounterForUserChannel = "UPDATE lastreadmessage "
                                                            "SET lastcounter = :counter "
                                                            "WHERE id = (SELECT l.id FROM lastreadmessage l "
                                                            "INNER JOIN users u ON u.id = l.userid "
                                                            "INNER JOIN channels c ON c.id = l.channelid "
                                                            "WHERE u.username = :user AND c.shaName = :shaName)";

static const QString selectLastReadMessageCount = "SELECT (COUNT(*) > 0) AS res FROM lastreadmessage "
                                                    "WHERE userid = :userid AND contactid = :contactid";

static const QString insertLastReadMessageRecord = "INSERT OR IGNORE INTO lastreadmessage "
                                                    "(lastcounter, userid, contactid, channelid) VALUES "
                                                    "(0, :userid, :contactid, :channelid)";

static const QString selectLastReadCountersForContacts = "SELECT c.username, l.lastcounter "
                                                            "FROM lastreadmessage l "
                                                            "INNER JOIN users u ON u.id = l.userid "
                                                            "INNER JOIN contacts c ON c.id = l.contactid "
                                                            "WHERE u.username = :user";

static const QString selectLastReadCountersForChannels = "SELECT c.shaName, l.lastcounter "
                                                            "FROM lastreadmessage l "
                                                            "INNER JOIN users u ON u.id = l.userid "
                                                            "INNER JOIN channels c ON c.id = l.channelid "
                                                            "WHERE u.username = :user";

static const QString selectChannelsWithLastReadCounters = "SELECT c.channel, c.shaName, c.adminName, c.isWriter, l.lastcounter "
                                                            "FROM channels c "
                                                            "LEFT JOIN lastreadmessage l ON l.channelid = c.id "
                                                            "INNER JOIN users u ON u.id = c.userid "
                                                            "WHERE u.username = :username "
                                                            "ORDER BY c.channel ASC";

static const QString insertMsgChannels = "INSERT INTO channels "
                                            "(userid, channel, shaName, isAdmin, adminName, isBanned, isWriter, isVisited) "
                                            "VALUES (:userid, :channel, :shaName, :isAdmin, :adminName, :isBanned, :isWriter, :isVisited)";

static const QString updateSetChannelsNotVisited = "UPDATE channels "
                                                    "SET isVisited = 0 "
                                                    "WHERE id = (SELECT id FROM users "
                                                    "WHERE username = :user)";

static const QString selectChannelForUserShaName = "SELECT c.id FROM channels c "
                                                    "INNER JOIN users u ON u.id = c.userid "
                                                    "WHERE u.username = :user "
                                                    "AND c.shaName = :shaName";

static const QString updateChannelInfo = "UPDATE channels "
                                        "SET isVisited = :isVisited "
                                        "WHERE id = :id";

static const QString updatetWriterForNotVisited = "UPDATE channels "
                                                    "SET isWriter = 1 "
                                                    "WHERE id = (SELECT id FROM users "
                                                    "WHERE username = :user) "
                                                    "AND isVisited = 0";

static const QString selectChannelInfoForUserShaName = "SELECT c.channel, c.shaName, c.adminName, c.isWriter "
                                                           "FROM channels c "
                                                           "INNER JOIN users u ON u.id = c.userid "
                                                           "WHERE u.username = :user "
                                                           "AND c.shaName = :shaName";

static const QString updateChannelIsWriterForUserShaName = "UPDATE channels "
                                                            "SET isWriter = :isWriter "
                                                            "WHERE id = (SELECT id FROM users "
                                                            "WHERE username = :user) "
                                                            "AND shaName = :shaName";

//static const QString selectWhereIsChannel = "AND m.channelid IS NOT NULL";
static const QString selectWhereIsNotChannel = "AND m.channelid IS NULL";
static const QString selectJoinChannel = "INNER JOIN channels c ON c.id = m.channelid AND c.shaName = :channelSha";

static const QString removeDecryptedDataQuery = "UPDATE messages "
                                        "SET isDecrypted = 0, decryptedText = \'\' "
                                        "WHERE isDecrypted = 1";

static const QString selectNotDecryptedMessagesContactsQuery = "SELECT m.id, u.username AS user, c.username AS dest, m.isIncoming, m.text, m.decryptedText, m.isDecrypted, "
                                                  "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                        "FROM messages m "
                                                        "INNER JOIN users u ON u.id = m.userid "
                                                        "INNER JOIN contacts c ON c.id = m.contactid "
                                                        "WHERE m.isDecrypted = 0 "
                                                        "AND u.username = :user "
                                                        "ORDER BY m.morder ASC";

static const QString selectNotDecryptedMessagesChannelsQuery = "SELECT m.id, u.username AS user, c.shaName AS dest, m.isIncoming, m.text, m.decryptedText, m.isDecrypted, "
                                                  "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                        "FROM messages m "
                                                        "INNER JOIN users u ON u.id = m.userid "
                                                        "INNER JOIN channels c ON c.id = m.channelid "
                                                        "WHERE m.isDecrypted = 0 "
                                                        "AND u.username = :user "
                                                        "ORDER BY m.morder ASC";

static const QString updateDecryptedMessageQuery = "UPDATE messages "
                                        "SET isDecrypted = :isDecrypted, decryptedText = :decryptedText "
                                        "WHERE id = :id";


}

#endif // MESSENGERDBRES_H
