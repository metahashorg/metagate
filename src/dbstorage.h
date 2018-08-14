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
    static DBStorage *instance() ;

    void init();

    void addPayment(const QString &id, const QString &transaction, const QString &from_account, const QString &to_account,
                    const QString &amount, const QString &value,
                    int block, bool is_input, int ts, const QString &confirmations);

    QList<QStringList> getPayments() const;

    void addMessage(const QString &user, const QString &duser,
                    const QString &text, uint64_t timestamp, Message::Counter counter,
                    bool isIncoming, bool canDecrypted, bool isConfirmed,
                    const QString &hash, qint64 fee);

    qint64 getUserId(const QString &username);
    QStringList getUsersList();

    qint64 getContactId(const QString &username);

    QString getUserPublicKey(const QString &username);
    void setUserPublicKey(const QString &username, const QString &publickey);

    QString getUserSignatures(const QString &username);
    void setUserSignatures(const QString &username, const QString &signatures);

    QString getContactrPublicKey(const QString &username);
    void setContactPublicKey(const QString &username, const QString &publickey);

    Message::Counter getMessageMaxCounter(const QString &user);
    Message::Counter getMessageMaxConfirmedCounter(const QString &user);

    std::list<Message> getMessagesForUser(const QString &user, qint64 from, qint64 to);
    std::list<Message> getMessagesForUserAndDest(const QString &user, const QString &duser, qint64 from, qint64 tos);
    std::list<Message> getMessagesForUserAndDestNum(const QString &user, const QString &duser, qint64 to, qint64 num);
    qint64 getMessagesCountForUserAndDest(const QString &user, const QString &duser, qint64 from);

    bool hasMessageWithCounter(const QString &username, Message::Counter counter);
    bool hasUnconfirmedMessageWithHash(const QString &hash);

    qint64 findFirstNotConfirmedMessageWithHash(const QString &hash);
    qint64 findFirstNotConfirmedMessage(const QString &username);
    void updateMessage(qint64 id, Message::Counter newCounter, bool confirmed);

    Message::Counter getLastReadCounterForUserContact(const QString &username, const QString &contact);
    void setLastReadCounterForUserContact(const QString &username, const QString &contact, Message::Counter counter);
    std::list<std::pair<QString, Message::Counter>> getLastReadCountersForUser(const QString &username);

private:
    explicit DBStorage(QObject *parent = nullptr);

    bool createTable(const QString &table, const QString &createQuery);
    void createMessagesList(QSqlQuery &query, std::list<Message> &messages, bool reverse = false);
    void addLastReadRecord(qint64 userid, qint64 contactid);

    QSqlDatabase m_db;
    bool m_dbExist;
};

#endif // DBSTORAGE_H
