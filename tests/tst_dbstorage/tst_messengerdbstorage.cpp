#include "tst_messengerdbstorage.h"

#include "messengerdbstorage.h"

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
    DBStorage::DbId id1 = db.getUserId("ddfjgjgj");
    DBStorage::DbId id2 = db.getUserId("ddfjgjgj");
    QCOMPARE(id1, id2);

    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 1500, true, true, true, "asdfdf", 1);
    QCOMPARE(db.getMessagesCountForUserAndDest("1234", "3454", 3000), 1);
    qDebug() << "answer " << db.getMessagesForUserAndDestNum("1234", "3454", 5000, 20).size();
    qDebug() << "answer " << db.getMessagesCountForUserAndDest("1234", "3454", 3000);
    qDebug() << db.getUserId("user1");
    qDebug() << db.getUserId("user2");
    qDebug() << db.getUserId("user3");
    qDebug() << db.getUserId("user1");
    qDebug() << db.getUserId("user5");
    qDebug() << db.getUserId("user6");

    db.addMessage("user6", "user7", "Hello!", 8458864, 1, true, true, true, "jkfjkjttrjkgfjkgfjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 1, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 2, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 3, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 4, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 5, true, true, true, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 6, true, true, false, "dfjkjkgfjkgfjkgfjkjk", 445);
    db.addMessage("user7", "user1", "Hello1!", 84583864, 7, true, true, false, "dfjkjkgfjkgfjkgfjkjk", 445);
    qDebug() << "?" << db.hasMessageWithCounter("1234", 4000);
    qDebug() << "?" << db.hasMessageWithCounter("1234", 2000);
    qDebug() << "?" << db.hasUnconfirmedMessageWithHash("1234", "asdfdf");
    qDebug() << "?" << db.hasUnconfirmedMessageWithHash("user7", "dfjkjkgfjkgfjkgfjkjk");

    /*
    qDebug() << "size" << db.getMessagesForUserAndDestNum("user7", "user1", 10, 1000).size();

    std::list<Message> msgs = db.getMessagesForUser("user7", 1, 3);
    qDebug() << "count " << msgs.size();

    qDebug() << db.getMessageMaxCounter("user7");
    qDebug() << db.getMessageMaxConfirmedCounter("user7");
    qDebug() << db.getMessageMaxConfirmedCounter("userururut");

    qDebug() << db.getUsersList();
*/
}

void tst_MessengerDBStorage::testQRCoderEncodeDecode_data()
{
    /*QTest::addColumn<QByteArray>("data");

    QTest::newRow("QRCoderEncodeDecode 1")
        << QByteArray::fromStdString(std::string("0009806da73b1589f38630649bdee48467946d118059efd6aab"));

    QTest::newRow("QRCoderEncodeDecode 2")
        << QByteArray(300, '\0');

    QTest::newRow("QRCoderEncodeDecode 3")
        <<  QByteArray(500, 'A');

    QTest::newRow("QRCoderEncodeDecode 4")
        << QByteArray();

    QTest::newRow("QRCoderEncodeDecode 5")
        << QByteArray("q");

    QTest::newRow("QRCoderEncodeDecode 6")
        << QByteArray::fromStdString(std::string("0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "\x00\x01\n\r"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"));

    QTest::newRow("QRCoderEncodeDecode 7")
        << QByteArray::fromStdString(std::string("12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"));
    QTest::newRow("QRCoderEncodeDecode 8")
        << QByteArray("0000\n\n\n\nn\nn");

    QTest::newRow("QRCoderEncodeDecode 9")
        << QByteArray("btc:L36xyLTQA4bEcFgF8aGvAcfoMBexMC3hAb25HmTBTnn8GahFVGQU 16RYK17Mwi27amr4nCR94uqWphLqm38FRY");

    QTest::newRow("QRCoderEncodeDecode 10")
        << QByteArray("tmh:f:1\nProc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,801b027ebf9b898380af57ed33594fe5\n\nFl8DM5t75F7Bucnf4b4jkf5H1zvXX1iPsxX2kp1kVRaMj7KGi//cau1oI5L9hrxT\n7yfJFMgIqTbwIi62czoMCJGBo4fh8FTK6XjRtOzPYx3dXkpaw7bIaJBP6PjXkM4D\nBIDfDdFYbuWn+WvZ//0eb2++SxKxmfDyOrW9AvJnuMs=");

    QTest::newRow("QRCoderEncodeDecode 11")
        << QByteArray("tmh:-----BEGIN EC PRIVATE KEY-----\nProc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,801b027ebf9b898380af57ed33594fe5\n\nFl8DM5t75F7Bucnf4b4jkf5H1zvXX1iPsxX2kp1kVRaMj7KGi//cau1oI5L9hrxT\n7yfJFMgIqTbwIi62czoMCJGBo4fh8FTK6XjRtOzPYx3dXkpaw7bIaJBP6PjXkM4D\nBIDfDdFYbuWn+WvZ//0eb2++SxKxmfDyOrW9AvJnuMs=\n-----END EC PRIVATE KEY-----\n\n");

    QTest::newRow("QRCoderEncodeDecode 12")
        << QByteArray("eth:{\"address\": \"c951ce32add35cc55b0ca1527e96a0fe36d6c2e9\",\"crypto\": {\"cipher\": \"aes-128-ctr\",\"ciphertext\": \"4c4e86aad46b6499ef76ebe05c1f40bed39290ddfa52be52eaab61fabbb3c89e\",\"cipherparams\": {\"iv\": \"41b7cc057e6384cc10915d2b14273971\"},\"kdf\": \"scrypt\",\"kdfparams\": {\"dklen\": 32,\"n\": 262144,\"p\": 1,\"r\": 8,\"salt\": \"aae8b7784e281077fac0f54b7bd661ab2205ae17ba55c5ce8e26475fc9615f78\"},\"mac\": \"c83846fc0962ad85169e778bfb6283b7d0cfd0632ed9b813b0f32aaa37990eea\"},\"id\": \"6fcda701-217b-4b0d-b1dd-14ea14542763\",\"version\": 3}");
        */
}

void tst_MessengerDBStorage::testQRCoderEncodeDecode()
{
    /*
    QFETCH(QByteArray, data);

    QByteArray bin = QRCoder::encode(data);
    QByteArray res = QRCoder::decode(bin);
    QCOMPARE(data, res);
    */
}

QTEST_MAIN(tst_MessengerDBStorage)
