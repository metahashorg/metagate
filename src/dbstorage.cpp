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

DBStorage::DBStorage(const QString &path, const QString &dbname, QObject *parent)
    : QObject(parent)
    , m_dbExist(false)
    , m_path(path)
    , m_dbName(dbname)
{
    openDB();
}

void DBStorage::init()
{
    if (dbExist())
        return;
    QSqlQuery query(sqliteSettings, m_db);
    query.exec();

    createTable(QStringLiteral("settings"), createSettingsTable);
    setSettings(QStringLiteral("dbversion"), "1"); // TODO
}

QString DBStorage::getSettings(const QString &key)
{
    QSqlQuery query(m_db);
    query.prepare(selectSettingsKeyValue);
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
    query.prepare(insertSettingsKeyValue);
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void DBStorage::setPath(const QString &path)
{
    m_path = path;
}

bool DBStorage::openDB()
{
    const QString pathToDB = makePath(m_path, m_dbName);

    m_dbExist = QFile::exists(pathToDB);
    m_db = QSqlDatabase::addDatabase("QSQLITE");
    m_db.setDatabaseName(pathToDB);
    if (!m_db.open()) {
        return false;
    }
    return true;
}

bool DBStorage::createTable(const QString &table, const QString &createQuery)
{
    QSqlQuery query(m_db);
    QString dropQuery = dropTable.arg(table);
    qDebug() << dropQuery;
    query.prepare(dropQuery);
    if (!query.exec()) {
        qDebug() << "DROP error" << query.lastError().text();
        return false;
    }

    query.prepare(createQuery);
    qDebug() << query.lastQuery();
    if (!query.exec()) {
        qDebug() << "CREATE error " << query.lastError().text();
        return  false;
    }
    return true;
}

bool DBStorage::createIndex(const QString &createQuery)
{
    QSqlQuery query(m_db);
    query.prepare(createQuery);
    qDebug() << query.lastQuery();
    if (!query.exec()) {
        qDebug() << "CREATE error " << query.lastError().text();
        return  false;
    }
    return true;
}

QSqlDatabase DBStorage::database() const
{
    return m_db;
}

bool DBStorage::dbExist() const
{
    return m_dbExist;
}
