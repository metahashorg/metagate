#ifndef DBSTORAGE_H
#define DBSTORAGE_H

#include <QObject>
#include <QSqlDatabase>

class DBStorage : public QObject
{
    Q_OBJECT
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

    explicit DBStorage(const QString &dbpath, const QString &dbname, QObject *parent = nullptr);
    virtual ~DBStorage();

    QString dbName() const;

    virtual void init(bool force);

    QString getSettings(const QString &key);
    void setSettings(const QString &key, const QString &value);

    void execPragma(const QString &sql);
    TransactionGuard beginTransaction();

protected:
    void setPath(const QString &path);
    void openDB();

    void createTable(const QString &table, const QString &createQuery);
    void createIndex(const QString &createQuery);
    QSqlDatabase database() const;
    bool dbExist() const;

private:
    QSqlDatabase m_db;
    bool m_dbExist;
    QString m_dbPath;
    QString m_dbName;
};

#endif // DBSTORAGE_H
