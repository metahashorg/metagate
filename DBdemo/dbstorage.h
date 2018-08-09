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

private:
    explicit DBStorage(QObject *parent = nullptr);

    QSqlDatabase m_db;
};

#endif // DBSTORAGE_H
