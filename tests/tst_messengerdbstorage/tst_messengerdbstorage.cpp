#include "tst_messengerdbstorage.h"

#include "MessengerDBStorage.h"

const QString dbName = "messenger.db";

tst_MessengerDBStorage::tst_MessengerDBStorage(QObject *parent)
    : QObject(parent)
{
}

void tst_MessengerDBStorage::testDB()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    MessengerDBStorage db;
    db.init();
    DBStorage::DbId id1 = db.getUserId("ddfjgjgj");
    DBStorage::DbId id2 = db.getUserId("ddfjgjgj");
    QCOMPARE(id1, id2);
}

void tst_MessengerDBStorage::testMessengerDB2()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    MessengerDBStorage db;
    db.init();

    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd123", 1, 1500, true, true, true, "asdfdf", 1);
    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "3454", 3000), 1);
    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "3454", 5000), 0);
    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "3454", 0), 2);

    std::vector<Message> r = db.getMessagesForUserAndDestNum("1234", "3454", 5000, 20);
    QCOMPARE(r.size(), 2);
    QCOMPARE(r.front().data, QStringLiteral("abcd123"));

    DBStorage::DbId id7 = db.getUserId("user7");

    db.addMessage("user6", "user7", "Hello!", 8458864, 1, true, true, true, "jkfjkjttrjkgfjkgfjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 1, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 2, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 3, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 4, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 5, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 6, true, true, false, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 7, true, true, false, "dfjkjkgfjkgfjkgfjkjk", 445);

    DBStorage::DbId id77 = db.getUserId("user7");
    QCOMPARE(id7, id77);

    QCOMPARE(db.hasMessageWithCounter("1234", 4000), true);
    QCOMPARE(db.hasMessageWithCounter("1234", 2000), false);
    QCOMPARE(db.hasMessageWithCounter("1234", 1500), true);

    QCOMPARE(db.hasUnconfirmedMessageWithHash("1234", "asdfdf"), false);
    QCOMPARE(db.hasUnconfirmedMessageWithHash("1234", "aoijkjsdfdf"), false);
    QCOMPARE(db.hasUnconfirmedMessageWithHash("556", "asdfdf"), false);
    QCOMPARE(db.hasUnconfirmedMessageWithHash("user7", "dfjkjkgfjkgfjkgfjkjk"), true);

    std::vector<Message> rr = db.getMessagesForUserAndDestNum("user7", "user1", 10, 1000);
    QCOMPARE(rr.size(), 7);
    qint64 pos[7] = {1, 2, 3, 4, 5, 6, 7};

    int k = 0;
    for (auto it = rr.begin(); it != rr.end (); ++it) {
        QCOMPARE(it->counter, pos[k]);
        QCOMPARE(it->collocutor, QStringLiteral("user1"));
        k++;
    }

    std::vector<Message> msgs = db.getMessagesForUser("user7", 1, 3);
    QCOMPARE(msgs.size(), 3);
    msgs = db.getMessagesForUser("user7", 1, 7);
    QCOMPARE(msgs.size(), 7);
    msgs = db.getMessagesForUser("user7", 4, 7);
    QCOMPARE(msgs.size(), 4);

    QCOMPARE(db.getMessageMaxCounter("user7"), 7);
    QCOMPARE(db.getMessageMaxCounter("user6"), 1);
    QCOMPARE(db.getMessageMaxCounter("1234"), 4000);
    //qDebug() << db.getMessageMaxCounter("user7");

    QCOMPARE(db.getMessageMaxConfirmedCounter("user7"), 5);
    QCOMPARE(db.getMessageMaxConfirmedCounter("userururut"), -1);
    //qDebug() << db.getUsersList();


    db.setUserPublicKey("user7", "dfkgflgfkltrioidfkldfklgfgf");
    QCOMPARE(db.getUserPublicKey("user7"), QStringLiteral("dfkgflgfkltrioidfkldfklgfgf"));
    QCOMPARE(db.getUserPublicKey("user1"), QStringLiteral(""));
    QCOMPARE(db.getUserPublicKey("userrrrr"), QStringLiteral(""));

    qint64 id = db.findFirstNotConfirmedMessage("user7");
    db.updateMessage(id, 4445, true);
    QVERIFY(id != db.findFirstNotConfirmedMessage("user7"));


    QCOMPARE(db.getLastReadCounterForUserContact("userrgjkg", "fjkgfjk"), -1);
    QCOMPARE(db.getLastReadCounterForUserContact("user7", "user1"), 0);
    db.setLastReadCounterForUserContact("user7", "user1", 244);
    QCOMPARE(db.getLastReadCounterForUserContact("user7", "user1"), 244);

    QCOMPARE(db.getLastReadCountersForContacts("user7").size(), 1);
}

void tst_MessengerDBStorage::testMessengerDBChannels()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    MessengerDBStorage db;
    db.init();
    DBStorage::DbId id1 = db.getUserId("1234");
    db.addChannel(id1, "channel", "jkgfjkgfgfitrrtoioriojk", true, "ktkt", false, true, true);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1, "jkgfjkgfgfitrrtoioriojk");
    db.addMessage("1234", "3454", "abcd", 1, 4001, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4002, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4003, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4004, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 1500, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 6000, true, true, true, "asdfdf", 1, "jkgfjkgfgfitrrtoioriojk");

    QCOMPARE(db.findFirstNotConfirmedMessageWithHash("1234", "asdfdf").second, -1);
    QCOMPARE(db.findFirstMessageWithHash("1234", "asdfdf").second, 1500);

    QCOMPARE(db.findFirstNotConfirmedMessageWithHash("1234", "asdfdf", "jkgfjkgfgfitrrtoioriojk").second, -1);
    QCOMPARE(db.findFirstMessageWithHash("1234", "asdfdf", "jkgfjkgfgfitrrtoioriojk").second, 4000);


    QCOMPARE(db.getMessageMaxCounter("1234"), 4004);
    QCOMPARE(db.getMessageMaxCounter("1234", "jkgfjkgfgfitrrtoioriojk"), 6000);
    QCOMPARE(db.hasMessageWithCounter("1234", 4000), false);
    QCOMPARE(db.hasMessageWithCounter("1234", 4001), true);
    QCOMPARE(db.hasMessageWithCounter("1234", 4000, "jkgfjkgfgfitrrtoioriojk"), true);
    QCOMPARE(db.hasMessageWithCounter("1234", 4001, "jkgfjkgfgfitrrtoioriojk"), false);
    //qDebug() << db.getMessageMaxConfirmedCounter("user7");
    //qDebug() << db.getMessageMaxConfirmedCounter("userururut");



}

QTEST_MAIN(tst_MessengerDBStorage)
