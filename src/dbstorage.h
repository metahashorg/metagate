#ifndef DBSTORAGE_H
#define DBSTORAGE_H

#include <QObject>
#include <QSqlDatabase>

#include <list>
#include <utility>
#include "Messenger/Message.h"

class DBStorage : public QObject
{
    Q_OBJECT
public:
    using DbId = qint64;
    using IdCounterPair = std::pair<DbId, Message::Counter>;

    explicit DBStorage(const QString &path, const QString &dbname, QObject *parent = nullptr);
    //static DBStorage *instance();

    virtual void init();

    QString getSettings(const QString &key);
    void setSettings(const QString &key, const QString &value);

protected:
    void setPath(const QString &path);
    bool openDB();

    bool createTable(const QString &table, const QString &createQuery);
    bool createIndex(const QString &createQuery);
    QSqlDatabase database() const;
    bool dbExist() const;

private:
    QSqlDatabase m_db;
    bool m_dbExist;
    QString m_path;
    QString m_dbName;
};

#endif // DBSTORAGE_H
