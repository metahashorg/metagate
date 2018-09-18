#include "dbstorage.h"

#include <QtSql>

#include "utils.h"
#include "check.h"

static const QString sqliteSettings = "PRAGMA foreign_keys=on";

static const QString dropTable = "DROP TABLE IF EXISTS %1";

static const QString createSettingsTable = "CREATE TABLE settings ( "
                                           "key VARCHAR(256) UNIQUE, "
                                           "value TEXT "
                                           ")";

static const QString insertSettingsKeyValue = "INSERT OR REPLACE INTO settings (key, value)"
                                             " VALUES (:key, :value)";

static const QString selectSettingsKeyValue = "SELECT value from SETTINGS WHERE key = :key";

DBStorage::DBStorage(const QString &dbpath, const QString &dbname, QObject *parent)
    : QObject(parent)
    , m_dbExist(false)
    , m_dbPath(dbpath)
    , m_dbName(dbname)
{
    openDB();
}

DBStorage::~DBStorage()
{
    m_db.close();
    m_db = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_dbName);
}

QString DBStorage::dbName() const
{
    return  m_dbName;
}

void DBStorage::init(bool force)
{
    if (dbExist() && !force)
        return;
    QSqlQuery query(sqliteSettings, m_db);
    CHECK(query.exec(), query.lastError().text().toStdString());

    createTable(QStringLiteral("settings"), createSettingsTable);
    setSettings(QStringLiteral("dbversion"), "1"); // TODO
}

QString DBStorage::getSettings(const QString &key)
{
    QSqlQuery query(m_db);
    CHECK(query.prepare(selectSettingsKeyValue), query.lastError().text().toStdString());
    query.bindValue(":key", key);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("value").toString();
    }
    return QString();
}

void DBStorage::setSettings(const QString &key, const QString &value)
{
    QSqlQuery query(m_db);
    CHECK(query.prepare(insertSettingsKeyValue), query.lastError().text().toStdString());
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void DBStorage::beginTransaction()
{
    CHECK(database().transaction(), "Transaction Begin");
}

void DBStorage::commitTransaction()
{
    CHECK(database().commit(), "Transaction Commit");
}

void DBStorage::rollbackTransaction()
{
    CHECK(database().rollback(), "Transaction Rollback");
}

void DBStorage::setPath(const QString &path)
{
    m_dbPath = path;
}

void DBStorage::openDB()
{
    const QString pathToDB = makePath(m_dbPath, m_dbName);

    m_dbExist = QFile::exists(pathToDB);
    m_db = QSqlDatabase::addDatabase("QSQLITE", m_dbName);
    m_db.setDatabaseName(pathToDB);
    CHECK(m_db.open(), "DB open error");
}

void DBStorage::createTable(const QString &table, const QString &createQuery)
{
    QSqlQuery query(m_db);
    QString dropQuery = dropTable.arg(table);
    CHECK(query.prepare(dropQuery), (table + QStringLiteral(" ") + query.lastError().text()).toStdString());
    CHECK(query.exec(), query.lastError().text().toStdString());

    CHECK(query.prepare(createQuery), (table + QStringLiteral(" ") + query.lastError().text()).toStdString());
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void DBStorage::createIndex(const QString &createQuery)
{
    QSqlQuery query(m_db);
    query.prepare(createQuery);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

QSqlDatabase DBStorage::database() const
{
    return m_db;
}

bool DBStorage::dbExist() const
{
    return m_dbExist;
}
