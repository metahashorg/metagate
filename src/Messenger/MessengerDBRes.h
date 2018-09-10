#ifndef MESSENGERDBRES_H
#define MESSENGERDBRES_H

#include <QString>

static const QString createMsgUsersTable = "CREATE TABLE msg_users ( "
                                           "id INTEGER PRIMARY KEY NOT NULL, "
                                           "username VARCHAR(100) UNIQUE, "
                                           "publickey VARCHAR(200), "
                                           "signatures VARCHAR(200) "
                                           ")";

static const QString createMsgContactsTable = "CREATE TABLE msg_contacts ( "
                                                "id INTEGER PRIMARY KEY NOT NULL, "
                                                "username VARCHAR(100) UNIQUE, "
                                                "publickey VARCHAR(200) "
                                                ")";

static const QString createMsgChannelsTable = "CREATE TABLE msg_channels ( "
                                                        "id INTEGER PRIMARY KEY NOT NULL, "
                                                        "userid  INTEGER NOT NULL, "
                                                        "channel TEXT, "
                                                        "shaName TEXT, "
                                                        "isAdmin BOOLEAN, "
                                                        "adminName VARCHAR(200), "
                                                        "isBanned BOOLEAN, "
                                                        "isWriter BOOLEAN, "
                                                        "isVisited BOOLEAN, "
                                                        "FOREIGN KEY (userid) REFERENCES msg_users(id) "
                                                        ")";


static const QString createMsgMessagesTable = "CREATE TABLE msg_messages ( "
                                           "id INTEGER PRIMARY KEY NOT NULL, "
                                           "userid  INTEGER NOT NULL, "
                                           "contactid  INTEGER, "
                                           "morder INT8, "
                                           "dt INT8, "
                                           "text TEXT, "
                                           "isIncoming BOOLEAN, "
                                           "canDecrypted BOOLEAN, "
                                           "isConfirmed BOOLEAN, "
                                           "hash VARCHAR(100), "
                                           "fee INT8, "
                                           "channelid  INTEGER, "
                                           "FOREIGN KEY (userid) REFERENCES msg_users(id), "
                                           "FOREIGN KEY (contactid) REFERENCES msg_contacts(id), "
                                           "FOREIGN KEY (channelid) REFERENCES msg_channels(id) "
                                           ")";

static const QString createMsgMessageUniqueIndex = "CREATE UNIQUE INDEX messagesUniqueIdx ON msg_messages ( "
                                                    "userid, contactid, morder, dt, text, isIncoming, canDecrypted, "
                                                    "isConfirmed, hash, fee)";

static const QString createMsgMessageCounterIndex = "CREATE INDEX messagesCounterIdx ON msg_messages(morder)";

static const QString createMsgLastReadMessageTable = "CREATE TABLE msg_lastreadmessage ( "
                                                        "id INTEGER PRIMARY KEY NOT NULL, "
                                                        "userid  INTEGER NOT NULL, "
                                                        "contactid  INTEGER, "
                                                        "channelid  INTEGER, "
                                                        "lastcounter INT8, "
                                                        "FOREIGN KEY (userid) REFERENCES msg_users(id), "
                                                        "FOREIGN KEY (contactid) REFERENCES msg_contacts(id), "
                                                        "FOREIGN KEY (channelid) REFERENCES msg_channels(id) "
                                                        ")";

static const QString selectMsgUsersForName = "SELECT id FROM msg_users WHERE username = :username";
static const QString insertMsgUsers = "INSERT INTO msg_users (username) VALUES (:username)";

static const QString selectMsgContactsForName = "SELECT id FROM msg_contacts WHERE username = :username";
static const QString insertMsgContacts = "INSERT INTO msg_contacts (username) VALUES (:username)";

static const QString insertMsgMessages = "INSERT OR IGNORE INTO msg_messages "
                                            "(userid, contactid, morder, dt, text, isIncoming, canDecrypted, isConfirmed, hash, fee, channelid) VALUES "
                                            "(:userid, :contactid, :order, :dt, :text, :isIncoming, :canDecrypted, :isConfirmed, :hash, :fee, :channelid)";

static const QString selectMsgMaxCounter = "SELECT IFNULL(MAX(m.morder), -1) AS max "
                                           "FROM msg_messages m "
                                           "INNER JOIN msg_users u ON u.id = m.userid %1 "
                                           "WHERE u.username = :user %2";

static const QString selectMsgMaxConfirmedCounter = "SELECT IFNULL(MAX(m.morder), -1) AS max "
                                                    "FROM msg_messages m "
                                                    "INNER JOIN msg_users u ON u.id = m.userid "
                                                    "WHERE m.isConfirmed = 1 "
                                                    "AND u.username = :user";

static QString selectMsgMessagesForUser = "SELECT u.username AS user, du.username AS duser, m.isIncoming, m.text, "
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
                                                       "AND u.username = :user AND du.username = :duser %1 "
                                                       "ORDER BY m.morder";

static const QString selectMsgMessagesForUserAndDestNum = "SELECT u.username AS user, du.username AS duser, m.isIncoming, m.text, "
                                                          "m.morder, m.dt, m.fee, m.canDecrypted, m.isConfirmed "
                                                          "FROM msg_messages m "
                                                          "INNER JOIN msg_users u ON u.id = m.userid "
                                                          "INNER JOIN msg_contacts du ON du.id = m.contactid "
                                                          "WHERE m.morder <= :oe "
                                                          "AND u.username = :user AND du.username = :duser %1 "
                                                          "ORDER BY m.morder DESC "
                                                          "LIMIT :num";


static const QString selectMsgUsersList = "SELECT username FROM msg_users ORDER BY id";

static const QString selectMsgUserPublicKey = "SELECT publickey FROM msg_users WHERE username = :user";
static const QString updateMsgUserPublicKey = "UPDATE msg_users SET publickey = :publickey WHERE username = :user";

static const QString selectMsgUserSignatures = "SELECT signatures FROM msg_users WHERE username = :user";
static const QString updateMsgUserSignatures = "UPDATE msg_users SET signatures = :signatures WHERE username = :user";

static const QString selectMsgContactsPublicKey = "SELECT publickey FROM msg_contacts WHERE username = :user";
static const QString updateMsgContactsPublicKey = "UPDATE msg_contacts SET publickey = :publickey WHERE username = :user";

static const QString selectMsgCountMessagesForUserAndDest = "SELECT COUNT(*) AS count "
                                                       "FROM msg_messages m "
                                                       "INNER JOIN msg_users u ON u.id = m.userid "
                                                       "INNER JOIN msg_contacts du ON du.id = m.contactid "
                                                       "WHERE m.morder >= :ob "
                                                       "AND u.username = :user AND du.username = :duser";


static const QString selectCountNotConfirmedMessagesWithHash = "SELECT (COUNT(*) > 0) AS res "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid "
                                                        "WHERE isConfirmed = 0 "
                                                        "AND hash = :hash "
                                                        "AND u.username = :user";

static const QString selectCountMessagesWithCounter = "SELECT (COUNT(*) > 0) AS res "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid %1 "
                                                        "WHERE m.morder = :counter "
                                                        "AND u.username = :user %2 ";


static const QString selectFirstNotConfirmedMessage = "SELECT m.id, m.morder "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid "
                                                        "WHERE m.isConfirmed = 0 "
                                                        "AND u.username = :user "
                                                        "ORDER BY m.morder "
                                                        "LIMIT 1";

static const QString selectFirstNotConfirmedMessageWithHash = "SELECT m.id, m.morder "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid %1 "
                                                        "WHERE m.isConfirmed = 0 "
                                                        "AND m.hash = :hash "
                                                        "AND u.username = :user %2 "
                                                        "ORDER BY m.morder "
                                                        "LIMIT 1";

static const QString selectFirstMessageWithHash = "SELECT m.id, m.morder "
                                                        "FROM msg_messages m "
                                                        "INNER JOIN msg_users u ON u.id = m.userid %1 "
                                                        "WHERE m.hash = :hash "
                                                        "AND u.username = :user %2 "
                                                        "ORDER BY m.morder "
                                                        "LIMIT 1";

static const QString updateMessageQuery = "UPDATE msg_messages "
                                        "SET isConfirmed = :isConfirmed, morder = :counter "
                                        "WHERE id = :id";

static const QString selectLastReadCounterForUserContact = "SELECT l.lastcounter FROM msg_lastreadmessage l "
                                                            "INNER JOIN msg_users u ON u.id = l.userid "
                                                            "INNER JOIN msg_contacts c ON c.id = l.contactid "
                                                            "WHERE u.username = :user AND c.username = :contact";

static const QString selectLastReadCounterForUserChannel = "SELECT l.lastcounter FROM msg_lastreadmessage l "
                                                            "INNER JOIN msg_users u ON u.id = l.userid "
                                                            "INNER JOIN msg_channels c ON c.id = l.channelid "
                                                            "WHERE u.username = :user AND c.shaName = :shaName";

static const QString updateLastReadCounterForUserContact = "UPDATE msg_lastreadmessage "
                                                            "SET lastcounter = :counter "
                                                            "WHERE id = (SELECT l.id FROM msg_lastreadmessage l "
                                                            "INNER JOIN msg_users u ON u.id = l.userid "
                                                            "INNER JOIN msg_contacts c ON c.id = l.contactid "
                                                            "WHERE u.username = :user AND c.username = :contact)";

static const QString updateLastReadCounterForUserChannel = "UPDATE msg_lastreadmessage "
                                                            "SET lastcounter = :counter "
                                                            "WHERE id = (SELECT l.id FROM msg_lastreadmessage l "
                                                            "INNER JOIN msg_users u ON u.id = l.userid "
                                                            "INNER JOIN msg_channels c ON c.id = l.channelid "
                                                            "WHERE u.username = :user AND c.shaName = :shaName)";

static const QString selectLastReadMessageCount = "SELECT (COUNT(*) > 0) AS res FROM msg_lastreadmessage "
                                                    "WHERE userid = :userid AND contactid = :contactid";
static const QString insertLastReadMessageRecord = "INSERT INTO msg_lastreadmessage "
                                                    "(lastcounter, userid, contactid) VALUES "
                                                    "(0, :userid, :contactid)";

static const QString selectLastReadCountersForContacts = "SELECT c.username, l.lastcounter "
                                                            "FROM msg_lastreadmessage l "
                                                            "INNER JOIN msg_users u ON u.id = l.userid "
                                                            "INNER JOIN msg_contacts c ON c.id = l.contactid "
                                                            "WHERE u.username = :user";

static const QString selectLastReadCountersForChannels = "SELECT c.shaName, l.lastcounter "
                                                            "FROM msg_lastreadmessage l "
                                                            "INNER JOIN msg_users u ON u.id = l.userid "
                                                            "INNER JOIN msg_channels c ON c.id = l.channelid "
                                                            "WHERE u.username = :user";

static const QString insertMsgChannels = "INSERT INTO msg_channels "
                                            "(userid, channel, shaName, isAdmin, adminName, isBanned, isWriter, isVisited) "
                                            "VALUES (:userid, :channel, :shaName, :isAdmin, :adminName, :isBanned, :isWriter, :isVisited)";

static const QString updateSetChannelsNotVisited = "UPDATE msg_channels "
                                                    "SET isVisited = 0 "
                                                    "WHERE id = (SELECT id FROM msg_users "
                                                    "WHERE username = :user)";

static const QString selectChannelForUserShaName = "SELECT c.id FROM msg_channels c "
                                                    "INNER JOIN msg_users u ON u.id = c.userid "
                                                    "WHERE u.username = :user "
                                                    "AND c.shaName = :shaName";

static const QString updateChannelInfo = "UPDATE msg_channels "
                                        "SET isVisited = :isVisited "
                                        "WHERE id = :id";

static const QString updatetWriterForNotVisited = "UPDATE msg_channels "
                                                    "SET isWriter = 1 "
                                                    "WHERE id = (SELECT id FROM msg_users "
                                                    "WHERE username = :user) "
                                                    "AND isVisited = 0";

static const QString selectChannelInfoForUserShaName = "SELECT c.channel, c.shaName, c.adminName, c.isWriter "
                                                           "FROM msg_channels c "
                                                           "INNER JOIN msg_users u ON u.id = c.userid "
                                                           "WHERE u.username = :user "
                                                           "AND c.shaName = :shaName";

static const QString updateChannelIsWriterForUserShaName = "UPDATE msg_channels "
                                                            "SET isWriter = :isWriter "
                                                            "WHERE id = (SELECT id FROM msg_users "
                                                            "WHERE username = :user) "
                                                            "AND shaName = :shaName";

static const QString selectWhereIsChannel = "AND m.channelid IS NOT NULL";
static const QString selectWhereIsNotChannel = "AND m.channelid IS NULL";
static const QString selectJoinChannel = "INNER JOIN msg_channels c ON c.id = m.channelid AND c.shaName = :channelSha";

#endif // MESSENGERDBRES_H
