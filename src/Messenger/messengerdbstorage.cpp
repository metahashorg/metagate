#include "messengerdbstorage.h"

#include "dbres.h"
#include <QtSql>

#include "check.h"

MessengerDBStorage::MessengerDBStorage()
    : DBStorage()
{

}

void MessengerDBStorage::init()
{
    if (dbExist())
        return;
    DBStorage::init();
    createTable(QStringLiteral("msg_users"), createMsgUsersTable);
    createTable(QStringLiteral("msg_contacts"), createMsgContactsTable);
    createTable(QStringLiteral("msg_messages"), createMsgMessagesTable);
    createTable(QStringLiteral("msg_lastreadmessage"), createMsgLastReadMessageTable);
    createTable(QStringLiteral("msg_channels"), createMsgChannelsTable);
    createIndex(createMsgMessageUniqueIndex);
}

void MessengerDBStorage::addMessage(const QString &user, const QString &duser, const QString &text, uint64_t timestamp, Message::Counter counter, bool isIncoming, bool canDecrypted, bool isConfirmed, const QString &hash, qint64 fee, const QString &channelSha)
{
    DbId userid = getUserId(user);
    DbId contactid = getContactId(duser);

    QSqlQuery query(database());
    query.prepare(insertMsgMessages);
    query.bindValue(":userid", userid);
    query.bindValue(":contactid", contactid);
    query.bindValue(":order", counter);
    query.bindValue(":dt", static_cast<qint64>(timestamp));
    query.bindValue(":text", text);
    query.bindValue(":isIncoming", isIncoming);
    query.bindValue(":canDecrypted", canDecrypted);
    query.bindValue(":isConfirmed", isConfirmed);
    query.bindValue(":hash", hash);
    query.bindValue(":fee", fee);
    if (!query.exec()) {
        qDebug() << "ERROR" << query.lastError().type();
    }
    //CHECK(query.exec(), query.lastError().text().toStdString());
    addLastReadRecord(userid, contactid);
}

DBStorage::DbId MessengerDBStorage::getUserId(const QString &username)
{
    QSqlQuery query(database());
    query.prepare(selectMsgUsersForName);
    query.bindValue(":username", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("id").toLongLong();
    }

    query.prepare(insertMsgUsers);
    query.bindValue(":username", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    return query.lastInsertId().toLongLong();
}

QStringList MessengerDBStorage::getUsersList()
{
    QStringList res;
    QSqlQuery query(database());
    query.prepare(selectMsgUsersList);
    CHECK(query.exec(), query.lastError().text().toStdString());
    while (query.next()) {
        res.push_back(query.value("username").toString());
    }
    return res;
}

DBStorage::DbId MessengerDBStorage::getContactId(const QString &username)
{
    QSqlQuery query(database());
    query.prepare(selectMsgContactsForName);
    query.bindValue(":username", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("id").toLongLong();
    }

    query.prepare(insertMsgContacts);
    query.bindValue(":username", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    return query.lastInsertId().toLongLong();
}

QString MessengerDBStorage::getUserPublicKey(const QString &username)
{
    QSqlQuery query(database());
    query.prepare(selectMsgUserPublicKey);
    query.bindValue(":user", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("publickey").toString();
    }
    return QString();
}

void MessengerDBStorage::setUserPublicKey(const QString &username, const QString &publickey)
{
    getUserId(username);
    QSqlQuery query(database());
    query.prepare(updateMsgUserPublicKey);
    query.bindValue(":user", username);
    query.bindValue(":publickey", publickey);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

QString MessengerDBStorage::getUserSignatures(const QString &username)
{
    QSqlQuery query(database());
    query.prepare(selectMsgUserSignatures);
    query.bindValue(":user", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("signatures").toString();
    }
    return QString();
}

void MessengerDBStorage::setUserSignatures(const QString &username, const QString &signatures)
{
    getUserId(username);
    QSqlQuery query(database());
    query.prepare(updateMsgUserSignatures);
    query.bindValue(":user", username);
    query.bindValue(":signatures", signatures);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

QString MessengerDBStorage::getContactrPublicKey(const QString &username)
{
    QSqlQuery query(database());
    query.prepare(selectMsgContactsPublicKey);
    query.bindValue(":user", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("publickey").toString();
    }
    return QString();
}

void MessengerDBStorage::setContactPublicKey(const QString &username, const QString &publickey)
{
    getContactId(username);
    QSqlQuery query(database());
    query.prepare(updateMsgContactsPublicKey);
    query.bindValue(":user", username);
    query.bindValue(":publickey", publickey);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

Message::Counter MessengerDBStorage::getMessageMaxCounter(const QString &user, const QString &channelSha)
{
    QSqlQuery query(database());
    query.prepare(selectMsgMaxCounter);
    query.bindValue(":user", user);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("max").toLongLong();
    }
    return -1;
}

Message::Counter MessengerDBStorage::getMessageMaxConfirmedCounter(const QString &user)
{
    QSqlQuery query(database());
    query.prepare(selectMsgMaxConfirmedCounter);
    query.bindValue(":user", user);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("max").toLongLong();
    }
    return -1;
}

std::list<Message> MessengerDBStorage::getMessagesForUser(const QString &user, qint64 from, qint64 to)
{
    std::list<Message> res;
    QSqlQuery query(database());
    query.prepare(selectMsgMessagesForUser);
    query.bindValue(":user", user);
    query.bindValue(":ob", from);
    query.bindValue(":oe", to);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createMessagesList(query, res);
    return res;
}

std::list<Message> MessengerDBStorage::getMessagesForUserAndDest(const QString &user, const QString &duser, qint64 from, qint64 to)
{
    std::list<Message> res;
    QSqlQuery query(database());
    query.prepare(selectMsgMessagesForUserAndDest);
    query.bindValue(":user", user);
    query.bindValue(":duser", duser);
    query.bindValue(":ob", from);
    query.bindValue(":oe", to);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createMessagesList(query, res);
    return res;
}

std::list<Message> MessengerDBStorage::getMessagesForUserAndDestNum(const QString &user, const QString &duser, qint64 to, qint64 num)
{
    std::list<Message> res;
    QSqlQuery query(database());
    query.prepare(selectMsgMessagesForUserAndDestNum);
    query.bindValue(":user", user);
    query.bindValue(":duser", duser);
    query.bindValue(":oe", to);
    query.bindValue(":num", num);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createMessagesList(query, res, true);
    return res;
}

qint64 MessengerDBStorage::getMessagesCountForUserAndDest(const QString &user, const QString &duser, qint64 from)
{
    QSqlQuery query(database());
    query.prepare(selectMsgCountMessagesForUserAndDest);
    query.bindValue(":user", user);
    query.bindValue(":duser", duser);
    query.bindValue(":ob", from);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("count").toLongLong();
    }
    return 0;
}

bool MessengerDBStorage::hasMessageWithCounter(const QString &username, Message::Counter counter, const QString &channelSha)
{
    QSqlQuery query(database());
    query.prepare(selectCountMessagesWithCounter);
    query.bindValue(":user", username);
    query.bindValue(":counter", counter);
    qDebug() << query.lastQuery();
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("res").toBool();
    }
    return false;
}

bool MessengerDBStorage::hasUnconfirmedMessageWithHash(const QString &username, const QString &hash)
{
    QSqlQuery query(database());
    query.prepare(selectCountNotConfirmedMessagesWithHash);
    query.bindValue(":user", username);
    query.bindValue(":hash", hash);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("res").toBool();
    }
    return false;
}

DBStorage::IdCounterPair MessengerDBStorage::findFirstNotConfirmedMessageWithHash(const QString &username, const QString &hash)
{
    QSqlQuery query(database());
    query.prepare(selectFirstNotConfirmedMessageWithHash);
    query.bindValue(":user", username);
    query.bindValue(":hash", hash);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return DBStorage::IdCounterPair(query.value("id").toLongLong(),
                                        query.value("morder").toLongLong());
    }
    return DBStorage::IdCounterPair(-1, -1);
}

DBStorage::IdCounterPair MessengerDBStorage::findFirstMessageWithHash(const QString &username, const QString &hash, const QString &channelSha)
{
    QSqlQuery query(database());
    query.prepare(selectFirstMessageWithHash);
    query.bindValue(":user", username);
    query.bindValue(":hash", hash);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return DBStorage::IdCounterPair(query.value("id").toLongLong(),
                                        query.value("morder").toLongLong());
    }
    return DBStorage::IdCounterPair(-1, -1);
}

DBStorage::DbId MessengerDBStorage::findFirstNotConfirmedMessage(const QString &username)
{
    QSqlQuery query(database());
    query.prepare(selectFirstNotConfirmedMessage);
    query.bindValue(":user", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("id").toLongLong();
    }
    return -1;
}

void MessengerDBStorage::updateMessage(DbId id, Message::Counter newCounter, bool confirmed, const QString &channelSha)
{
    QSqlQuery query(database());
    query.prepare(updateMessageQuery);
    query.bindValue(":id", id);
    query.bindValue(":counter", newCounter);
    query.bindValue(":isConfirmed", confirmed);
    query.exec();
    qDebug() << query.lastError().text();
    //CHECK(query.exec(), query.lastError().text().toStdString());
}

Message::Counter MessengerDBStorage::getLastReadCounterForUserContact(const QString &username, const QString &contact, bool isChannel)
{
    QSqlQuery query(database());
    query.prepare(selectLastReadCounterForUserContact);
    query.bindValue(":user", username);
    query.bindValue(":contact", contact);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("lastcounter").toLongLong();
    }
    return -1;
}

void MessengerDBStorage::setLastReadCounterForUserContact(const QString &username, const QString &contact, Message::Counter counter, bool isChannel)
{
    QSqlQuery query(database());
    query.prepare(updateLastReadCounterForUserContact);
    query.bindValue(":counter", counter);
    query.bindValue(":user", username);
    query.bindValue(":contact", contact);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

std::list<std::pair<QString, Message::Counter> > MessengerDBStorage::getLastReadCountersForUser(const QString &username, bool isChannel)
{
    std::list<std::pair<QString, Message::Counter> > res;
    QSqlQuery query(database());
    query.prepare(selectLastReadCountersForUser);
    query.bindValue(":user", username);
    CHECK(query.exec(), query.lastError().text().toStdString());
    while (query.next()) {
        std::pair<QString, Message::Counter> p(query.value("username").toString(), query.value("lastcounter").toLongLong());
        res.push_back(p);
    }
    return res;
}

void MessengerDBStorage::addChannel(DBStorage::DbId userid, const QString &channel, const QString &shaName, bool isAdmin, const QString &adminName, bool isBanned, bool isWriter, bool isVisited)
{
    QSqlQuery query(database());
    query.prepare(insertMsgChannels);
    query.bindValue(":userid", userid);
    query.bindValue(":channel", channel);
    query.bindValue(":shaName", shaName);
    query.bindValue(":isAdmin", isAdmin);
    query.bindValue(":adminName", adminName);
    query.bindValue(":isBanned", isBanned);
    query.bindValue(":isWriter", isWriter);
    query.bindValue(":isVisited", isVisited);
    qDebug() << query.lastQuery();
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void MessengerDBStorage::setChannelsNotVisited(const QString &user)
{
    QSqlQuery query(database());
    query.prepare(updateSetChannelsNotVisited);
    query.bindValue(":user", user);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

DBStorage::DbId MessengerDBStorage::getChannelForUserShaName(const QString &user, const QString &shaName)
{
    QSqlQuery query(database());
    query.prepare(selectChannelForUserShaName);
    query.bindValue(":user", user);
    query.bindValue(":shaName", shaName);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("id").toLongLong();
    }
    return -1;
}

void MessengerDBStorage::updateChannel(DBStorage::DbId id, bool isVisited)
{
    QSqlQuery query(database());
    query.prepare(updateChannelInfo);
    query.bindValue(":id", id);
    query.bindValue(":isVisited", isVisited);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void MessengerDBStorage::setWriterForNotVisited(const QString &user)
{
    QSqlQuery query(database());
    query.prepare(updatetWriterForNotVisited);
    query.bindValue(":user", user);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void MessengerDBStorage::getChannelInfoForUserShaName(const QString &user, const QString &shaName)
{
    QSqlQuery query(database());
    query.prepare(selectChannelInfoForUserShaName);
    query.bindValue(":user", user);
    query.bindValue(":shaName", shaName);
    CHECK(query.exec(), query.lastError().text().toStdString());
//    if (query.next()) {
//        return query.value("max").toLongLong();
    //    }
}

void MessengerDBStorage::setChannelIsWriterForUserShaName(const QString &user, const QString &shaName, bool isWriter)
{
    QSqlQuery query(database());
    query.prepare(updateChannelIsWriterForUserShaName);
    query.bindValue(":user", user);
    query.bindValue(":shaName", shaName);
    query.bindValue(":isWriter", isWriter);
    qDebug() << query.lastQuery();
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void MessengerDBStorage::createMessagesList(QSqlQuery &query, std::list<Message> &messages, bool reverse)
{
    while (query.next()) {
        Message msg;
        msg.collocutor = query.value("duser").toString();
        msg.isInput = query.value("isIncoming").toBool();
        msg.data = query.value("text").toString();
        msg.counter = query.value("morder").toLongLong();
        msg.timestamp = static_cast<quint64>(query.value("dt").toLongLong());
        msg.fee = query.value("fee").toLongLong();
        msg.isCanDecrypted = query.value("canDecrypted").toBool();
        msg.isConfirmed = query.value("isConfirmed").toBool();
        if (reverse)
            messages.push_front(msg);
        else
            messages.push_back(msg);
    }
}

void MessengerDBStorage::addLastReadRecord(DBStorage::DbId userid, DBStorage::DbId contactid)
{
    QSqlQuery query(database());
    query.prepare(selectLastReadMessageCount);
    query.bindValue(":userid", userid);
    query.bindValue(":contactid", contactid);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (!query.next()) {
        return;
    }
    qint64 count = query.value("res").toLongLong();
    if (count > 0)
        return;
    query.prepare(insertLastReadMessageRecord);
    query.bindValue(":userid", userid);
    query.bindValue(":contactid", contactid);
    CHECK(query.exec(), query.lastError().text().toStdString());
}
