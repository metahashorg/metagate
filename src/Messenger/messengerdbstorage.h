#ifndef MESSENGERDBSTORAGE_H
#define MESSENGERDBSTORAGE_H

#include "dbstorage.h"

class MessengerDBStorage : public DBStorage
{
public:
    MessengerDBStorage();

    virtual void init() override;

    void addMessage(const QString &user, const QString &duser,
                    const QString &text, uint64_t timestamp, Message::Counter counter,
                    bool isIncoming, bool canDecrypted, bool isConfirmed,
                    const QString &hash, qint64 fee, const QString &channelSha = QString());

    DbId getUserId(const QString &username);
    QStringList getUsersList();

    DbId getContactId(const QString &username);

    QString getUserPublicKey(const QString &username);
    void setUserPublicKey(const QString &username, const QString &publickey);

    QString getUserSignatures(const QString &username);
    void setUserSignatures(const QString &username, const QString &signatures);

    QString getContactrPublicKey(const QString &username);
    void setContactPublicKey(const QString &username, const QString &publickey);

    Message::Counter getMessageMaxCounter(const QString &user, const QString &channelSha = QString());
    Message::Counter getMessageMaxConfirmedCounter(const QString &user);

    std::list<Message> getMessagesForUser(const QString &user, qint64 from, qint64 to);
    std::list<Message> getMessagesForUserAndDest(const QString &user, const QString &duser, qint64 from, qint64 tos);
    std::list<Message> getMessagesForUserAndDestNum(const QString &user, const QString &duser, qint64 to, qint64 num);
    qint64 getMessagesCountForUserAndDest(const QString &user, const QString &duser, qint64 from);

    bool hasMessageWithCounter(const QString &username, Message::Counter counter, const QString &channelSha = QString());
    bool hasUnconfirmedMessageWithHash(const QString &username, const QString &hash);

    IdCounterPair findFirstNotConfirmedMessageWithHash(const QString &username, const QString &hash);
    IdCounterPair findFirstMessageWithHash(const QString &username, const QString &hash, const QString &channelSha = QString());
    DbId findFirstNotConfirmedMessage(const QString &username);
    void updateMessage(DbId id, Message::Counter newCounter, bool confirmed, const QString &channelSha = QString());

    Message::Counter getLastReadCounterForUserContact(const QString &username, const QString &contact, bool isChannel = false);
    void setLastReadCounterForUserContact(const QString &username, const QString &contact, Message::Counter counter, bool isChannel = false);
    std::list<std::pair<QString, Message::Counter>> getLastReadCountersForUser(const QString &username, bool isChannel = false);

    void addChannel(DbId userid, const QString &channel, const QString &shaName, bool isAdmin, const QString &adminName, bool isBanned, bool isWriter, bool isVisited);
    void setChannelsNotVisited(const QString &user);
    DbId getChannelForUserShaName(const QString &user, const QString &shaName);
    void updateChannel(DbId id, bool isVisited);
    void setWriterForNotVisited(const QString &user);

    // TODO
    void getChannelInfoForUserShaName(const QString &user, const QString &shaName);
    void setChannelIsWriterForUserShaName(const QString &user, const QString &shaName, bool isWriter);

private:
    void createMessagesList(QSqlQuery &query, std::list<Message> &messages, bool reverse = false);
    void addLastReadRecord(DbId userid, DbId contactid);
};

#endif // MESSENGERDBSTORAGE_H
