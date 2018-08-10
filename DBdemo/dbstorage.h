#ifndef DBSTORAGE_H
#define DBSTORAGE_H

#include <QObject>
#include <QSqlDatabase>

class DBStorage : public QObject
{
    Q_OBJECT
public:
    static DBStorage *instance() ;

    void init();

    void addPayment(const QString &id, const QString &transaction, const QString &from_account, const QString &to_account,
                    const QString &amount, const QString &value,
                    int block, bool is_input, int ts, const QString &confirmations);

    QList<QStringList> getPayments() const;

    void addMessage(const QString &user, const QString &duser,
                    const QString &text, qint64 dt, qint64 order,
                    bool isIncoming, bool canDecrypted, bool isConfirmed,
                    const QString &hash);

    qint64 getUserId(const QString &username);

private:
    explicit DBStorage(QObject *parent = nullptr);

    bool createTable(const QString &table, const QString &createQuery);

    QSqlDatabase m_db;
};

#endif // DBSTORAGE_H
