#include "dbstorage.h"
#include "dbres.h"

#include <QtSql>

#include "check.h"
#include "Paths.h"
#include "utils.h"

DBStorage *DBStorage::instance()
{
    static DBStorage self;
    return &self;
}

void DBStorage::init()
{
    if (m_dbExist)
        return;
    QSqlQuery query(sqliteSettings, m_db);
    query.exec();

    /*QSqlQuery query(dropPaymentsTable);
    if (!query.exec()) {
        qDebug() << "DROP error" << query.lastError().text();
    }*/

//    QSqlQuery query1(createPaymentsTable);
//    if (!query1.exec()) {

//        qDebug() << "CREATE error " << query1.lastError().text();
//    }

    createTable(QStringLiteral("settings"), createSettingsTable);
    createTable(QStringLiteral("msg_users"), createMsgUsersTable);
    createTable(QStringLiteral("msg_contacts"), createMsgContactsTable);
    createTable(QStringLiteral("msg_messages"), createMsgMessagesTable);
    createTable(QStringLiteral("msg_lastreadmessage"), createMsgLastReadMessageTable);
}

void DBStorage::addPayment(const QString &id, const QString &transaction, const QString &from_account, const QString &to_account, const QString &amount, const QString &value, int block, bool is_input, int ts, const QString &confirmations)
{
    QSqlQuery query(m_db);
    query.prepare(preparePaymentsInsert);
    query.bindValue(":id", id);
    query.bindValue(":trans", transaction);
    query.bindValue(":from_account", from_account);
    query.bindValue(":to_account", to_account);
    query.bindValue(":amount", amount);
    query.bindValue(":value", value);
    query.bindValue(":block", block);
    query.bindValue(":is_input", is_input);
    query.bindValue(":ts", ts);
    query.bindValue(":confirmations", confirmations);
    if (!query.exec()) {
        qDebug() << "ERROR " <<  query.lastError().text();
    }


}

QList<QStringList> DBStorage::getPayments() const
{
    QList<QStringList> res;
    QSqlQuery query(selectPayments, m_db);
    qDebug() << query.lastQuery();
    if (!query.exec()) {
        qDebug() << "ERROR " <<  query.lastError().text();

    }
    while (query.next()) {
        QStringList r;
        QString id = query.value(0).toString();
        QString transaction = query.value(1).toString();
        QString from_account = query.value(2).toString();
        QString to_account = query.value(3).toString();
        r << id << transaction << from_account << to_account;
        r << query.value(4).toString();
        r << query.value(5).toString();
        r << query.value(6).toString();
        r << query.value(7).toString();
        r << query.value(8).toString();
        r << query.value(9).toString();
        res.append(r);
    }
    return res;
}

void DBStorage::addMessage(const QString &user, const QString &duser, const QString &text, uint64_t timestamp, Message::Counter counter, bool isIncoming, bool canDecrypted, bool isConfirmed, const QString &hash, qint64 fee)
{
    qint64 userid = getUserId(user);
    qint64 contactid = getContactId(duser);

    QSqlQuery query(m_db);
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
        qDebug() << "INSERT error " << query.lastError().text();
    }
    qDebug() << query.lastInsertId().toLongLong();
    addLastReadRecord(userid, contactid);
}

qint64 DBStorage::getUserId(const QString &username)
{
    QSqlQuery query(m_db);
    query.prepare(selectMsgUsersForName);
    query.bindValue(":username", username);
    if (!query.exec()) {

    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }

    query.prepare(insertMsgUsers);
    query.bindValue(":username", username);
    if (!query.exec()) {
        qDebug() << "INSERT error " << query.lastError().text();
    }
    return query.lastInsertId().toLongLong();
}

QStringList DBStorage::getUsersList()
{
    QStringList res;
    QSqlQuery query(m_db);
    if (!query.exec(selectMsgUsersList)) {

    }
    while (query.next()) {
        res.push_back(query.value(0).toString());
    }
    return res;
}

qint64 DBStorage::getContactId(const QString &username)
{
    QSqlQuery query(m_db);
    query.prepare(selectMsgContactsForName);
    query.bindValue(":username", username);
    if (!query.exec()) {

    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }

    query.prepare(insertMsgContacts);
    query.bindValue(":username", username);
    if (!query.exec()) {
        qDebug() << "INSERT error " << query.lastError().text();
    }
    return query.lastInsertId().toLongLong();
}

QString DBStorage::getUserPublicKey(const QString &username)
{
    QSqlQuery query(m_db);
    query.prepare(selectMsgUserPublicKey);
    query.bindValue(":user", username);
    if (!query.exec()) {
        return QString();
    }
    if (query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

void DBStorage::setUserPublicKey(const QString &username, const QString &publickey)
{
    getUserId(username);
    QSqlQuery query(m_db);
    query.prepare(updateMsgUserPublicKey);
    query.bindValue(":user", username);
    query.bindValue(":publickey", publickey);
    if (!query.exec()) {
    }
}

QString DBStorage::getUserSignatures(const QString &username)
{
    QSqlQuery query(m_db);
    query.prepare(selectMsgUserSignatures);
    query.bindValue(":user", username);
    if (!query.exec()) {
        return QString();
    }
    if (query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

void DBStorage::setUserSignatures(const QString &username, const QString &signatures)
{
    getUserId(username);
    QSqlQuery query(m_db);
    query.prepare(updateMsgUserSignatures);
    query.bindValue(":user", username);
    query.bindValue(":signatures", signatures);
    if (!query.exec()) {
    }
}

QString DBStorage::getContactrPublicKey(const QString &username)
{
    QSqlQuery query(m_db);
    query.prepare(selectMsgContactsPublicKey);
    query.bindValue(":user", username);
    if (!query.exec()) {
        return QString();
    }
    if (query.next()) {
        return query.value(0).toString();
    }
    return QString();
}

void DBStorage::setContactPublicKey(const QString &username, const QString &publickey)
{
    getContactId(username);
    QSqlQuery query(m_db);
    query.prepare(updateMsgContactsPublicKey);
    query.bindValue(":user", username);
    query.bindValue(":publickey", publickey);
    if (!query.exec()) {
    }
}

Message::Counter DBStorage::getMessageMaxCounter(const QString &user)
{
    QSqlQuery query(m_db);
    query.prepare(selectMsgMaxCounter);
    query.bindValue(":user", user);
    if (!query.exec()) {
        return -1;
    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}

Message::Counter DBStorage::getMessageMaxConfirmedCounter(const QString &user)
{
    QSqlQuery query(m_db);
    query.prepare(selectMsgMaxConfirmedCounter);
    query.bindValue(":user", user);
    if (!query.exec()) {
        return -1;
    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}

std::list<Message> DBStorage::getMessagesForUser(const QString &user, qint64 from, qint64 to)
{
    std::list<Message> res;
    QSqlQuery query(m_db);
    query.prepare(selectMsgMessagesForUser);
    query.bindValue(":user", user);
    query.bindValue(":ob", from);
    query.bindValue(":oe", to);
    if (!query.exec()) {

    }
    createMessagesList(query, res);
    return res;
}

std::list<Message> DBStorage::getMessagesForUserAndDest(const QString &user, const QString &duser, qint64 from, qint64 to)
{
    std::list<Message> res;
    QSqlQuery query(m_db);
    query.prepare(selectMsgMessagesForUserAndDest);
    query.bindValue(":user", user);
    query.bindValue(":duser", duser);
    query.bindValue(":ob", from);
    query.bindValue(":oe", to);
    if (!query.exec()) {

    }
    createMessagesList(query, res);
    return res;
}

std::list<Message> DBStorage::getMessagesForUserAndDestNum(const QString &user, const QString &duser, qint64 to, qint64 num)
{
    std::list<Message> res;
    QSqlQuery query(m_db);
    query.prepare(selectMsgMessagesForUserAndDestNum);
    query.bindValue(":user", user);
    query.bindValue(":duser", duser);
    query.bindValue(":oe", to);
    query.bindValue(":num", num);
    qDebug() << query.lastQuery();
    if (!query.exec()) {

    }
    createMessagesList(query, res, true);
    return res;
}

qint64 DBStorage::getMessagesCountForUserAndDest(const QString &user, const QString &duser, qint64 from)
{
    QSqlQuery query(m_db);
    query.prepare(selectMsgCountMessagesForUserAndDest);
    query.bindValue(":user", user);
    query.bindValue(":duser", duser);
    query.bindValue(":ob", from);
    if (!query.exec()) {
        return 0;
    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }
    return 0;
}

bool DBStorage::hasMessageWithCounter(const QString &username, Message::Counter counter)
{
    QSqlQuery query(m_db);
    query.prepare(selectCountMessagesWithCounter);
    query.bindValue(":user", username);
    query.bindValue(":counter", counter);
    if (!query.exec()) {

    }
    if (query.next()) {
        return query.value(0).toLongLong() > 0;
    }
    return false;
}

bool DBStorage::hasUnconfirmedMessageWithHash(const QString &username, const QString &hash)
{
    QSqlQuery query(m_db);
    query.prepare(selectCountNotConfirmedMessagesWithHash);
    query.bindValue(":user", username);
    query.bindValue(":hash", hash);
    if (!query.exec()) {

    }
    if (query.next()) {
        return query.value(0).toLongLong() > 0;
    }
    return false;
}

qint64 DBStorage::findFirstNotConfirmedMessageWithHash(const QString &username, const QString &hash)
{
    QSqlQuery query(m_db);
    query.prepare(selectFirstNotConfirmedMessageWithHash);
    query.bindValue(":user", username);
    query.bindValue(":hash", hash);
    if (!query.exec()) {
        return -1;
    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}

qint64 DBStorage::findFirstNotConfirmedMessage(const QString &username)
{
    QSqlQuery query(m_db);
    query.prepare(selectFirstNotConfirmedMessage);
    query.bindValue(":user", username);
    if (!query.exec()) {
        return -1;
    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}

void DBStorage::updateMessage(qint64 id, Message::Counter newCounter, bool confirmed)
{
    QSqlQuery query(m_db);
    query.prepare(updateMessageQuery);
    query.bindValue(":id", id);
    query.bindValue(":counter", newCounter);
    query.bindValue(":isConfirmed", confirmed);
    if (!query.exec()) {
    }
}

Message::Counter DBStorage::getLastReadCounterForUserContact(const QString &username, const QString &contact)
{
    QSqlQuery query(m_db);
    query.prepare(selectLastReadCounterForUserContact);
    query.bindValue(":user", username);
    query.bindValue(":contact", contact);
    if (!query.exec()) {
        return -1;
    }
    if (query.next()) {
        return query.value(0).toLongLong();
    }
    return -1;
}

void DBStorage::setLastReadCounterForUserContact(const QString &username, const QString &contact, Message::Counter counter)
{
    QSqlQuery query(m_db);
    query.prepare(updateLastReadCounterForUserContact);
    query.bindValue(":counter", counter);
    query.bindValue(":user", username);
    query.bindValue(":contact", contact);
    if (!query.exec()) {
    }
}

std::list<std::pair<QString, Message::Counter> > DBStorage::getLastReadCountersForUser(const QString &username)
{
    std::list<std::pair<QString, Message::Counter> > res;
    QSqlQuery query(m_db);
    query.prepare(selectLastReadCountersForUser);
    query.bindValue(":user", username);
    if (!query.exec()) {
        return res;
    }
    while (query.next()) {
        std::pair<QString, Message::Counter> p(query.value(0).toString(), query.value(1).toLongLong());
        res.push_back(p);
    }
    return res;
}

DBStorage::DBStorage(QObject *parent)
    : QObject(parent)
    , m_dbExist(false)
{
    //QSqlDatabase db;
    const QString pathToBd = makePath(getBdPath(), "database.db");

    m_dbExist = QFile::exists(pathToBd);
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(pathToBd);
    if (m_db.open())
        qDebug() << "DB ok";
    else
        qDebug() << "DB open error";
}

bool DBStorage::createTable(const QString &table, const QString &createQuery)
{
    QString dropQuery = dropTable.arg(table);
    qDebug() << dropQuery;
    QSqlQuery dquery(dropQuery, m_db);
    if (!dquery.exec()) {
        qDebug() << "DROP error" << dquery.lastError().text();
        return false;
    }

    QSqlQuery cquery(createQuery, m_db);
    qDebug() << cquery.lastQuery();
    if (!cquery.exec()) {
        qDebug() << "CREATE error " << cquery.lastError().text();
        return  false;
    }
    return true;
}

void DBStorage::createMessagesList(QSqlQuery &query, std::list<Message> &messages, bool reverse)
{
    while (query.next()) {
        Message msg;
        msg.collocutor = query.value(1).toString();
        msg.isInput = query.value(2).toBool();
        msg.data = query.value(3).toString();
        msg.counter = query.value(4).toLongLong();
        msg.timestamp = static_cast<quint64>(query.value(5).toLongLong());
        msg.fee = query.value(6).toLongLong();
        msg.isCanDecrypted = query.value(7).toBool();
        msg.isConfirmed = query.value(8).toBool();
        if (reverse)
            messages.push_front(msg);
        else
            messages.push_back(msg);
    }
}

void DBStorage::addLastReadRecord(qint64 userid, qint64 contactid)
{
    QSqlQuery query(m_db);
    query.prepare(selectLastReadMessageCount);
    query.bindValue(":userid", userid);
    query.bindValue(":contactid", contactid);
    if (!query.exec()) {

    }
    if (!query.next()) {
        return;
    }
    qint64 count = query.value(0).toLongLong();
    if (count > 0)
        return;
    query.prepare(insertLastReadMessageRecord);
    query.bindValue(":userid", userid);
    query.bindValue(":contactid", contactid);
    if (!query.exec()) {
        qDebug() << "INSERT error " << query.lastError().text();
    }
    qDebug() << "ok";
}
