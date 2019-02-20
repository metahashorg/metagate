#ifndef DBSTORAGE_H
#define DBSTORAGE_H

#include <QSqlDatabase>
#include <QVariant>

class DBStorage {
public:

    class TransactionGuard {
    public:

        TransactionGuard(const DBStorage &storage);

        ~TransactionGuard();

        TransactionGuard(TransactionGuard &&second);

        TransactionGuard(const TransactionGuard &second) = delete;
        TransactionGuard& operator=(const TransactionGuard &second) = delete;
        TransactionGuard& operator=(TransactionGuard &&second) = delete;

        void commit();

    private:

        const DBStorage &storage;
        bool isClose = false;
        bool isCommited = false;
    };

public:
    using DbId = qint64;

    const static DbId not_found;

    explicit DBStorage(const QString &dbpath, const QString &dbname);
    virtual ~DBStorage();

    QString dbName() const;
    QString dbFileName() const;
    virtual int currentVersion() const = 0;

    bool init();

    QVariant getSettings(const QString &key);
    void setSettings(const QString &key, const QVariant &value);

    void execPragma(const QString &sql);
    TransactionGuard beginTransaction();

protected:
    void setPath(const QString &path);
    void openDB();

    virtual void createDatabase() = 0;
    void createTable(const QString &table, const QString &createQuery);
    void createIndex(const QString &createQuery);
    QSqlDatabase database() const;
    bool dbExist() const;

private:
    bool updateDB();
    void updateToNewVersion(int vcur, int vnew);
    void execFromFile(const QString &filename);

    QSqlDatabase m_db;
    bool m_dbExist;
    QString m_dbPath;
    QString m_dbName;
};

#endif // DBSTORAGE_H
