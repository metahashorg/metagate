#include "dbstorage.h"

#include <QtSql>

#include "check.h"

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
                                           "username VARCHAR(100) "
                                           ")";

static const QString createMsgMessagesTable = "CREATE TABLE msg_messages ( "
                                           "id INTEGER PRIMARY KEY NOT NULL, "
                                           "userid  INTEGER NOT NULL, "
                                           "duserid  INTEGER NOT NULL, "
                                           "morder INT8, "
                                           "dt INT8, "
                                           "text TEXT, "
                                           "isIncoming BOOLEAN, "
                                           "canDecrypted BOOLEAN, "
                                           "isConfirmed BOOLEAN, "
                                           "hash VARCHAR(100), "
                                           "FOREIGN KEY (userid) REFERENCES msg_users(id), "
                                           "FOREIGN KEY (duserid) REFERENCES msg_users(id) "
                                           ")";

static const QString selectMsgUsersForName = "SELECT id FROM msg_users WHERE username = :username";
static const QString insertMsgUsers = "INSERT INTO msg_users (username) VALUES (:username)";

static const QString insertMsgMessages = "INSERT INTO msg_messages "
                                            "(userid, duserid, morder, dt, text, isIncoming, canDecrypted, isConfirmed, hash) VALUES "
                                            "(:userid, :duserid, :order, :dt, :text, :isIncoming, :canDecrypted, :isConfirmed, :hash)";

static const QString selectMsgMaxCounter = "SELECT IFNULL(MAX(m.morder), -1) AS max "
                                           "FROM msg_messages m "
                                           "INNER JOIN msg_users u ON u.id = m.userid "
                                           "WHERE u.username = :user";

static const QString selectMsgMaxConfirmedCounter = "SELECT IFNULL(MAX(m.morder), -1) AS max "
                                                    "FROM msg_messages m "
                                                    "INNER JOIN msg_users u ON u.id = m.userid "
                                                    "WHERE m.isConfirmed = 1 "
                                                    "AND u.username = :user";

static const QString selectMsgMessagesForUser = "SELECT u.username AS user, du.username AS duser, m.isIncoming, m.text, m.morder, m.dt "
                                                "FROM msg_messages m "
                                                "INNER JOIN msg_users u ON u.id = m.userid "
                                                "INNER JOIN msg_users du ON du.id = m.duserid "
                                                "WHERE m.morder >= :ob AND m.morder <= :oe "
                                                "AND u.username = :user "
                                                "ORDER BY m.morder";

static const QString selectMsgMessagesForUserAndDest = "SELECT u.username AS user, du.username AS duser, m.isIncoming, m.text, m.morder, m.dt "
                                                       "FROM msg_messages m "
                                                       "INNER JOIN msg_users u ON u.id = m.userid "
                                                       "INNER JOIN msg_users du ON du.id = m.duserid "
                                                       "WHERE m.morder >= :ob AND m.morder <= :oe "
                                                       "AND u.username = :user AND du.username = :duser "
                                                       "ORDER BY m.morder";

static const QString selectMsgMessagesForUserAndDestNum = "SELECT u.username AS user, du.username AS duser, m.isIncoming, m.text, m.morder, m.dt "
                                                          "FROM msg_messages m "
                                                          "INNER JOIN msg_users u ON u.id = m.userid "
                                                          "INNER JOIN msg_users du ON du.id = m.duserid "
                                                          "WHERE m.morder >= :ob "
                                                          "AND u.username = :user AND du.username = :duser "
                                                          "ORDER BY m.morder "
                                                          "LIMIT :num";

DBStorage *DBStorage::instance()
{
    static DBStorage self;
    return &self;
}

void DBStorage::init()
{
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
    createTable(QStringLiteral("msg_messages"), createMsgMessagesTable);
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

void DBStorage::addMessage(const QString &user, const QString &duser, const QString &text, uint64_t timestamp, Message::Counter counter, bool isIncoming, bool canDecrypted, bool isConfirmed, const QString &hash)
{
    qint64 userid = getUserId(user);
    qint64 duserid = getUserId(duser);

    QSqlQuery query(m_db);
    query.prepare(insertMsgMessages);
    query.bindValue(":userid", userid);
    query.bindValue(":duserid", duserid);
    query.bindValue(":order", counter);
    query.bindValue(":dt", static_cast<qint64>(timestamp));
    query.bindValue(":text", text);
    query.bindValue(":isIncoming", isIncoming);
    query.bindValue(":canDecrypted", canDecrypted);
    query.bindValue(":isConfirmed", isConfirmed);
    query.bindValue(":hash", hash);

    if (!query.exec()) {
        qDebug() << "INSERT error " << query.lastError().text();
    }
    qDebug() << query.lastInsertId().toLongLong();
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

std::list<Message> DBStorage::getMessagesForUser(const QString &user, qint64 ob, qint64 oe)
{
    std::list<Message> res;
    QSqlQuery query(m_db);
    query.prepare(selectMsgMessagesForUser);
    query.bindValue(":user", user);
    query.bindValue(":ob", ob);
    query.bindValue(":oe", oe);
    if (!query.exec()) {

    }
    createMessagesList(query, res);
    return res;
}

std::list<Message> DBStorage::getMessagesForUserAndDest(const QString &user, const QString &duser, qint64 ob, qint64 oe)
{
    std::list<Message> res;
    QSqlQuery query(m_db);
    query.prepare(selectMsgMessagesForUserAndDest);
    query.bindValue(":user", user);
    query.bindValue(":duser", duser);
    query.bindValue(":ob", ob);
    query.bindValue(":oe", oe);
    if (!query.exec()) {

    }
    createMessagesList(query, res);
    return res;
}

std::list<Message> DBStorage::getMessagesForUserAndDestNum(const QString &user, const QString &duser, qint64 ob, qint64 num)
{
    std::list<Message> res;
    QSqlQuery query(m_db);
    query.prepare(selectMsgMessagesForUserAndDestNum);
    query.bindValue(":user", user);
    query.bindValue(":duser", duser);
    query.bindValue(":ob", ob);
    query.bindValue(":num", num);
    if (!query.exec()) {

    }
    createMessagesList(query, res);
    return res;
}

DBStorage::DBStorage(QObject *parent)
    : QObject(parent)
{
    //QSqlDatabase db;
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName("database.db");
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

void DBStorage::createMessagesList(QSqlQuery &query, std::list<Message> &messages)
{
    while (query.next()) {
        Message msg;
        msg.collocutor = query.value(1).toString();
        msg.isInput = query.value(2).toBool();
        msg.data = query.value(3).toString();
        msg.counter = query.value(4).toLongLong();
        msg.timestamp = static_cast<quint64>(query.value(5).toLongLong());
        messages.push_back(msg);
    }
}
