#include "tst_messengerdbstorage.h"

#include <QTest>

#include <iostream>

#include "check.h"

#include "Messenger/MessengerDBStorage.h"

const QString dbName = "messenger.db";

tst_MessengerDBStorage::tst_MessengerDBStorage(QObject *parent)
    : QObject(parent)
{
}

void tst_MessengerDBStorage::testDB()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    messenger::MessengerDBStorage db;
    db.init();
    DBStorage::DbId id1 = db.getUserId("ddfjgjgj");
    DBStorage::DbId id2 = db.getUserId("ddfjgjgj");
    QCOMPARE(id1, id2);
}

void tst_MessengerDBStorage::testMessengerDB2()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    messenger::MessengerDBStorage db;
    db.init();

    db.setUserPublicKey("1234", "23424", "2345342", "", "");

    db.addMessage("1234", "3454", "abcd", "", false, 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd123", "", false, 1, 1500, true, true, true, "asdfdf", 1, QString(""));
    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "3454", 3000), 1);
    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "3454", 5000), 0);
    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "3454", 0), 2);

    std::vector<messenger::Message> r = db.getMessagesForUserAndDestNum("1234", "3454", 5000, 20);
    QCOMPARE(r.size(), 2);
    QCOMPARE(r.front().dataHex, QStringLiteral("abcd123"));

    db.setUserPublicKey("user6", "23424", "2345342", "", "");
    db.setUserPublicKey("user7", "23424", "2345342", "", "");
    DBStorage::DbId id7 = db.getUserId("user7");
    db.addMessage("user6", "user7", "Hello!", "", false, 8458864, 1, true, true, true, "jkfjkjttrjkgfjkgfjk", 445);
    db.addMessage("user7", "user1", "Hello1!", "", false, 84583864, 1, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", "", false, 84583864, 2, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", "", false, 84583864, 3, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", "", false, 84583864, 4, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", "", false, 84583864, 5, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", "", false, 84583864, 6, true, true, false, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", "", false, 84583864, 7, true, true, false, "dfjkjkgfjkgfjkgfjkjk", 445);

    DBStorage::DbId id77 = db.getUserId("user7");
    QCOMPARE(id7, id77);

    QCOMPARE(db.hasMessageWithCounter("1234", 4000), true);
    QCOMPARE(db.hasMessageWithCounter("1234", 2000), false);
    QCOMPARE(db.hasMessageWithCounter("1234", 1500), true);

    QCOMPARE(db.hasUnconfirmedMessageWithHash("1234", "asdfdf"), false);
    QCOMPARE(db.hasUnconfirmedMessageWithHash("1234", "aoijkjsdfdf"), false);
    QCOMPARE(db.hasUnconfirmedMessageWithHash("556", "asdfdf"), false);
    QCOMPARE(db.hasUnconfirmedMessageWithHash("user7", "dfjkjkgfjkgfjkgfjkjk"), true);

    std::vector<messenger::Message> rr = db.getMessagesForUserAndDestNum("user7", "user1", 10, 1000);
    QCOMPARE(rr.size(), 7);
    qint64 pos[7] = {1, 2, 3, 4, 5, 6, 7};

    int k = 0;
    for (auto it = rr.begin(); it != rr.end (); ++it) {
        QCOMPARE(it->counter, pos[k]);
        QCOMPARE(it->collocutor, QStringLiteral("user1"));
        k++;
    }

    std::vector<messenger::Message> msgs = db.getMessagesForUser("user7", 1, 3);
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

    db.setUserPublicKey("user7", "dfkgflgfkltrioidfkldfklgfgf", "dsafdasf", "1234", "5678");
    QCOMPARE(db.getUserPublicKey("user7"), QStringLiteral("dfkgflgfkltrioidfkldfklgfgf"));
    QCOMPARE(db.getUserPublicKey("user1"), QStringLiteral(""));
    QCOMPARE(db.getUserPublicKey("userrrrr"), QStringLiteral(""));
    const auto userInfo = db.getUserInfo("user7");
    QCOMPARE(userInfo.pubkeyRsa, QStringLiteral("dsafdasf"));
    QCOMPARE(userInfo.txRsaHash, QStringLiteral("1234"));
    QCOMPARE(userInfo.blockchainName, QStringLiteral("5678"));

    const auto userInfo3 = db.getUserInfo("user77");
    QCOMPARE(userInfo3.pubkeyRsa, QStringLiteral(""));
    QCOMPARE(userInfo3.txRsaHash, QStringLiteral(""));
    QCOMPARE(userInfo3.blockchainName, QStringLiteral(""));

    db.setContactPublicKey("user27", "pubkey1", "tx1", "bl1");
    const auto userInfo2 = db.getContactInfo("user27");
    QCOMPARE(userInfo2.pubkeyRsa, QStringLiteral("pubkey1"));
    QCOMPARE(userInfo2.txRsaHash, QStringLiteral("tx1"));
    QCOMPARE(userInfo2.blockchainName, QStringLiteral("bl1"));

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
    messenger::MessengerDBStorage db;
    db.init();
    db.setUserPublicKey("1234", "23424", "2345342", "", "");
    DBStorage::DbId id1 = db.getUserId("1234");
    db.addChannel(id1, "channel", "jkgfjkgfgfitrrtoioriojk", true, "ktkt", false, true, true);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4000, true, true, true, "asdfdf", 1, "jkgfjkgfgfitrrtoioriojk");
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4001, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4002, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3457", "abcd", "", false, 1, 4003, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4004, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 1500, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 6000, true, true, true, "asdfdf", 1, "jkgfjkgfgfitrrtoioriojk");

    std::vector<messenger::Message> rr = db.getMessagesForUserAndDestNum("1234", "jkgfjkgfgfitrrtoioriojk", 10000, 1000, true);
    QCOMPARE(rr.size(), 2);

    QCOMPARE(db.findFirstNotConfirmedMessageWithHash("1234", "asdfdf").second, -1);
    QCOMPARE(db.findFirstMessageWithHash("1234", "asdfdf").second, 1500);

    QCOMPARE(db.findFirstNotConfirmedMessageWithHash("1234", "asdfdf", "jkgfjkgfgfitrrtoioriojk").second, -1);
    QCOMPARE(db.findFirstMessageWithHash("1234", "asdfdf", "jkgfjkgfgfitrrtoioriojk").second, 4000);

    db.addMessage("1234", "3454", "abcd", "", false, 1, 1501, true, true, false, "asdfdf", 1);
    QCOMPARE(db.findFirstNotConfirmedMessageWithHash("1234", "asdfdf").second, 1501);

    QCOMPARE(db.getLastReadCounterForUserContact("1234", "jkgfjkgfgfitrrtoioriojk", true), 0);
    db.setLastReadCounterForUserContact("1234", "jkgfjkgfgfitrrtoioriojk", 4567, true);
    QCOMPARE(db.getLastReadCounterForUserContact("1234", "jkgfjkgfgfitrrtoioriojk", true), 4567);
    db.setLastReadCounterForUserContact("1234", "jkgfjkgfgfitrrtoioriojk", 17, true);
    QCOMPARE(db.getLastReadCounterForUserContact("1234", "jkgfjkgfgfitrrtoioriojk", true), 17);


    QCOMPARE(db.getLastReadCounterForUserContact("1234", "3454", false), 0);
    QCOMPARE(db.getLastReadCounterForUserContact("1234", "3457", false), 0);
    db.setLastReadCounterForUserContact("1234", "3454", 44322, false);
    QCOMPARE(db.getLastReadCounterForUserContact("1234", "3457", false), 0);
    QCOMPARE(db.getLastReadCounterForUserContact("1234", "3454", false), 44322);
    db.setLastReadCounterForUserContact("1234", "3454", 42, false);
    db.setLastReadCounterForUserContact("1234", "3457", 452, false);
    QCOMPARE(db.getLastReadCounterForUserContact("1234", "3457", false), 452);
    QCOMPARE(db.getLastReadCounterForUserContact("1234", "3454", false), 42);

    QCOMPARE(db.getMessageMaxCounter("1234"), 4004);
    QCOMPARE(db.getMessageMaxCounter("1234", "jkgfjkgfgfitrrtoioriojk"), 6000);
    QCOMPARE(db.hasMessageWithCounter("1234", 4000), false);
    QCOMPARE(db.hasMessageWithCounter("1234", 4001), true);
    QCOMPARE(db.hasMessageWithCounter("1234", 4000, "jkgfjkgfgfitrrtoioriojk"), true);
    QCOMPARE(db.hasMessageWithCounter("1234", 4001, "jkgfjkgfgfitrrtoioriojk"), false);
    //qDebug() << db.getMessageMaxConfirmedCounter("user7");
    //qDebug() << db.getMessageMaxConfirmedCounter("userururut");


    db.addChannel(id1, "channel1", "0564", true, "admin", false, true, true);
    db.setLastReadCounterForUserContact("1234", "0564", 10, true);

    const std::vector<messenger::ChannelInfo> channels = db.getChannelsWithLastReadCounters("1234");
    QCOMPARE(channels.size(), 2);
    for (const messenger::ChannelInfo &channel: channels) {
        if (channel.title == "channel1") {
            QCOMPARE(channel.counter, 10);
        }
    }
}

void tst_MessengerDBStorage::testMessengerDBSpeed()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    messenger::MessengerDBStorage db;
    db.init();
    db.setUserPublicKey("1234", "23424", "2345342", "", "");
    auto transactionGuard = db.beginTransaction();
    for (int n = 0; n < 1000; n++) {
        db.addMessage("1234", "3454", "abcd", "", false, 1000000 + n, 4001 + n, true, true, true, "asdfdf", 1);
    }
    transactionGuard.commit();
    qDebug() << db.getMessageMaxCounter("1234");
}

void tst_MessengerDBStorage::testMessengerDecryptedText()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    messenger::MessengerDBStorage db;
    db.init();

    db.setUserPublicKey("1234", "23424", "2345342", "", "");
    DBStorage::DbId id1 = db.getUserId("1234");
    db.addChannel(id1, "channel1", "ch1", true, "ktkt", false, true, true);
    db.addChannel(id1, "channel2", "ch2", true, "ktkt", false, true, true);
    db.addMessage("1234", "3454", "abcd", "", false, 1, 4000, true, true, true, "asdfdf", 1, "ch1");
    db.addMessage("1234", "3454", "abcd2", "", false, 1, 4001, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "34546", "abcd", "sadfads", true, 1, 4002, true, true, true, "asdfdf", 1, "ch2");
    db.addMessage("1234", "34546", "abcdadfas", "fdsfd", true, 1, 4003, true, true, true, "asdfdf", 1);

    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "3454", 3000), 1);
    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "34546", 3000), 1);

    {
        std::vector<messenger::Message> r = db.getMessagesForUserAndDestNum("1234", "3454", 10000, 20);
        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].isDecrypted, false);
    }

    {
        std::vector<messenger::Message> r = db.getMessagesForUserAndDestNum("1234", "ch1", 10000, 20, true);
        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].isDecrypted, false);
    }

    {
        std::vector<messenger::Message> r = db.getMessagesForUserAndDestNum("1234", "34546", 10000, 20);
        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].isDecrypted, true);
        QCOMPARE(r[0].decryptedDataHex, "fdsfd");
    }

    {
        std::vector<messenger::Message> r = db.getMessagesForUserAndDestNum("1234", "ch2", 10000, 20, true);
        QCOMPARE(r.size(), 1);
        QCOMPARE(r[0].isDecrypted, true);
        QCOMPARE(r[0].decryptedDataHex, "sadfads");
    }

    {
        std::vector<messenger::Message> r = db.getMessagesForUser("1234", 1, 10000);
        QCOMPARE(r.size(), 2);
        QCOMPARE(r[0].isDecrypted, false);
        QCOMPARE(r[1].isDecrypted, true);
        QCOMPARE(r[1].decryptedDataHex, "fdsfd");
    }

    {
        auto r = db.getNotDecryptedMessage("1234");
        QCOMPARE(r.second.size(), 2);
        QCOMPARE(r.second[0].isDecrypted, false);
        QCOMPARE(r.second[1].isDecrypted, false);
        QCOMPARE(r.second[0].decryptedDataHex, "");
        QCOMPARE(r.second[1].decryptedDataHex, "");
    }

    db.removeDecryptedData();
    {
        std::vector<messenger::Message> r = db.getMessagesForUser("1234", 1, 10000);
        QCOMPARE(r.size(), 2);
        QCOMPARE(r[0].isDecrypted, false);
        QCOMPARE(r[1].isDecrypted, false);
        QCOMPARE(r[0].decryptedDataHex, "");
        QCOMPARE(r[1].decryptedDataHex, "");
    }

    {
        auto r = db.getNotDecryptedMessage("1234");
        QCOMPARE(r.second.size(), 4);
        QCOMPARE(r.second[0].isDecrypted, false);
        QCOMPARE(r.second[1].isDecrypted, false);
        QCOMPARE(r.second[0].decryptedDataHex, "");
        QCOMPARE(r.second[1].decryptedDataHex, "");
        QCOMPARE(r.second[2].isDecrypted, false);
        QCOMPARE(r.second[3].isDecrypted, false);
        QCOMPARE(r.second[2].decryptedDataHex, "");
        QCOMPARE(r.second[3].decryptedDataHex, "");

        db.updateDecryptedMessage({{r.first[0], true, "sdafdasf"}, {r.first[1], false, ""}, {r.first[3], true, "ereeer"}});

        {
            auto r = db.getNotDecryptedMessage("1234");
            QCOMPARE(r.second.size(), 2);
            QCOMPARE(r.second[0].isDecrypted, false);
            QCOMPARE(r.second[1].isDecrypted, false);
            QCOMPARE(r.second[0].decryptedDataHex, "");
            QCOMPARE(r.second[1].decryptedDataHex, "");
        }

        {
            std::vector<messenger::Message> r = db.getMessagesForUser("1234", 1, 10000);
            QCOMPARE(r.size(), 2);
            QCOMPARE(r[0].isDecrypted, true);
            QCOMPARE(r[1].isDecrypted, false);
            QCOMPARE(r[0].decryptedDataHex, "sdafdasf");
        }

        {
            std::vector<messenger::Message> r = db.getMessagesForUserAndDestNum("1234", "ch2", 10000, 20, true);
            QCOMPARE(r.size(), 1);
            QCOMPARE(r[0].isDecrypted, true);
            QCOMPARE(r[0].decryptedDataHex, "ereeer");
        }
    }
}

QTEST_MAIN(tst_MessengerDBStorage)
