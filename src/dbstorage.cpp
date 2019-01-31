#include "dbstorage.h"

#include <QtSql>

#include "utils.h"
#include "check.h"
#include "Log.h"

static const QString dbFileNameSuffix = "db";

static const QString sqliteSettings = "PRAGMA foreign_keys=on";

static const QString dropTable = "DROP TABLE IF EXISTS %1";

static const QString createSettingsTable = "CREATE TABLE settings ( "
                                           "key VARCHAR(256) UNIQUE, "
                                           "value TEXT "
                                           ")";

static const QString insertSettingsKeyValue = "INSERT OR REPLACE INTO settings (key, value)"
                                             " VALUES (:key, :value)";

static const QString selectSettingsKeyValue = "SELECT value from SETTINGS WHERE key = :key";

static const QString settingsDBVersion = "dbversion";
static const QString updatesLocationPrefix = ":/";

DBStorage::DBStorage(const QString &dbpath, const QString &dbname)
    : m_dbExist(false)
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
    return m_dbName;
}

QString DBStorage::dbFileName() const
{
    return QString("%1.%2").arg(m_dbName).arg(dbFileNameSuffix);
}

bool DBStorage::init()
{
    if (dbExist()) {
        return updateDB();
    }
    LOG << "Create DB " << dbName();
    // Create settings
    QSqlQuery query(sqliteSettings, m_db);
    CHECK(query.exec(), query.lastError().text().toStdString());
    createTable(QStringLiteral("settings"), createSettingsTable);
    setSettings(settingsDBVersion, currentVersion());

    createDatabase();
    return true;
}

QVariant DBStorage::getSettings(const QString &key)
{
    QSqlQuery query(m_db);
    CHECK(query.prepare(selectSettingsKeyValue), query.lastError().text().toStdString());
    query.bindValue(":key", key);
    CHECK(query.exec(), query.lastError().text().toStdString());
    if (query.next()) {
        return query.value("value");
    }
    return QString();
}

void DBStorage::setSettings(const QString &key, const QVariant &value)
{
    QSqlQuery query(m_db);
    CHECK(query.prepare(insertSettingsKeyValue), query.lastError().text().toStdString());
    query.bindValue(":key", key);
    query.bindValue(":value", value);
    CHECK(query.exec(), query.lastError().text().toStdString());
}

void DBStorage::execPragma(const QString &sql)
{
    database().exec(sql);
}

DBStorage::TransactionGuard DBStorage::beginTransaction() {
    return TransactionGuard(*this);
}

void DBStorage::setPath(const QString &path)
{
    m_dbPath = path;
}

void DBStorage::openDB()
{
    const QString pathToDB = makePath(m_dbPath, dbFileName());

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

bool DBStorage::updateDB()
{
    int ver = getSettings(settingsDBVersion).toInt();
    int nver = currentVersion();
    LOG << "Database " << dbName() << " app's version " << ver << " new version " << nver;
    if (ver == nver)
        return true;
    if (ver > nver)
        return false; //DB version greater than current
    auto transactionGuard = beginTransaction();
    for (int v = ver; v < nver; v++) {
        updateToNewVersion(v, v + 1);
    }
    setSettings(settingsDBVersion, nver);
    transactionGuard.commit();
    return true;
}

void DBStorage::updateToNewVersion(int vcur, int vnew)
{
    CHECK(vcur + 1 == vnew, "possible update to incremented version");
    LOG << "Update " << dbName() << " version " << vcur << "->" << vnew;
    QString filename = updatesLocationPrefix + QStringLiteral("%1_%2to%3.sql").arg(dbName()).arg(vcur).arg(vnew);
    execFromFile(filename);
}

void DBStorage::execFromFile(const QString &filename)
{
    LOG << "DB update " << filename;
    QFile file(filename);

    CHECK(file.open(QIODevice::ReadOnly | QIODevice::Text), "can't open file")
    QTextStream in(&file);
    QString data = in.readAll();
    QStringList sqls = data.split(';');
    QSqlQuery query(m_db);
    for (const QString &sql : sqls) {
        if (sql.trimmed().isEmpty())
            continue;
        CHECK(query.prepare(sql), query.lastError().text().toStdString());
        CHECK(query.exec(), query.lastError().text().toStdString());
    }
}

DBStorage::TransactionGuard::TransactionGuard(const DBStorage &storage)
    : storage(storage)
{
    CHECK(storage.database().transaction(), "Transaction not open");

    isClose = true;
}

DBStorage::TransactionGuard::~TransactionGuard() {
    if (isClose) {
        if (!storage.database().rollback()) {
            LOG << "Error while rollback db commit";
        }
    }
}

DBStorage::TransactionGuard::TransactionGuard(DBStorage::TransactionGuard &&second)
    : storage(second.storage)
    , isClose(second.isClose)
    , isCommited(second.isCommited)
{
    second.isCommited = true;
    second.isClose = false;
}

void DBStorage::TransactionGuard::commit() {
    CHECK(!isCommited, "already commited");
    CHECK(storage.database().commit(), "Transaction not commit");
    isCommited = true;
    isClose = false;
}
