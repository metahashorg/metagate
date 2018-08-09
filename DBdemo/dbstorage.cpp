#include "dbstorage.h"

#include <QtSql>

static const QString dropPaymentsTable = "DROP TABLE IF EXISTS payments";

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

DBStorage *DBStorage::instance()
{
    static DBStorage self;
    return &self;
}

void DBStorage::init()
{
    QSqlQuery query(dropPaymentsTable);
    if (!query.exec()) {
        qDebug() << "DROP error" << query.lastError().text();
    }

    QSqlQuery query1(createPaymentsTable);
    if (!query1.exec()) {

        qDebug() << "CREATE error " << query1.lastError().text();
    }
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
