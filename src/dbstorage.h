#ifndef DBSTORAGE_H
#define DBSTORAGE_H

#include <QObject>
#include <QSqlDatabase>

#include <list>
#include "Messenger/Message.h"

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
                    const QString &text, uint64_t timestamp, Message::Counter counter,
                    bool isIncoming, bool canDecrypted, bool isConfirmed,
                    const QString &hash);

    qint64 getUserId(const QString &username);
    QStringList getUsersList();

    QString getUserPublicKey(const QString &username);
    void setUserPublicKey(const QString &username, const QString &publickey);

    Message::Counter getMessageMaxCounter(const QString &user);
    Message::Counter getMessageMaxConfirmedCounter(const QString &user);

    std::list<Message> getMessagesForUser(const QString &user, qint64 from, qint64 to);
    std::list<Message> getMessagesForUserAndDest(const QString &user, const QString &duser, qint64 from, qint64 tos);
    std::list<Message> getMessagesForUserAndDestNum(const QString &user, const QString &duser, qint64 from, qint64 num);

    qint64 findLastNotConfirmedMessage(const QString &username);
    void updateMessage(qint64 id, Message::Counter newCounter, bool confirmed);

private:
    explicit DBStorage(QObject *parent = nullptr);

    bool createTable(const QString &table, const QString &createQuery);
    void createMessagesList(QSqlQuery &query, std::list<Message> &messages);

    QSqlDatabase m_db;
};

#endif // DBSTORAGE_H
