#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFile>
#include <QTreeWidget>
#include "dbstorage.h"
#include "MessengerDBStorage.h"
#include "TransactionsDBStorage.h"
#include "SlotWrapper.h"
#include "BigNumber.h"
#include "Transaction.h"

#include "client.h"
#include <QDebug>

#include <list>
#include <vector>
#include "HttpClient.h"
#include <iostream>

#include <QSettings>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);


BEGIN_SLOT_WRAPPER
    /*if (QFile::exists("messenger.db"))
        QFile::remove("messenger.db");
    if (QFile::exists("payments.db"))
        QFile::remove("payments.db");

    {
        transactions::TransactionsDBStorage tdb;
        tdb.init();
        auto transactionGuard = tdb.beginTransaction();
        for (qint64 n = 0; n < 3000; n++) {
            tdb.addPayment("mh", QString("gfklklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address100", true, "user7", "user1", "9000000000000000000", 1000 + 2 * n, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
        }
        transactionGuard.commit();
    }*/
    {
        transactions::TransactionsDBStorage tdb;
        tdb.init();
        std::vector<qint64> r = tdb.getBlockNumbers();
        qDebug() << r.size();

    }

    return;


    {
            messenger::MessengerDBStorage db;

    //db.openDB();
    db.init();
    std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
    auto transactionGuard = db.beginTransaction();
    //db.addChannel(id1, "channel", "jkgfjkgfgfitrrtoioriojk", true, "ktkt", false, true, true);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1, "jkgfjkgfgfitrrtoioriojk");
    transactionGuard.commit();
    std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();

    std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<std::endl;
    }
    {
        transactions::TransactionsDBStorage tdb;
        tdb.init();
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();

        auto transactionGuard = tdb.beginTransaction();
        tdb.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", true, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
        transactionGuard.commit();
        std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();

        std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<std::endl;

    }
END_SLOT_WRAPPER
    return;

    /*{
        QList<TestStructure> list;
        TestStructure t;
        t.name = "John Smith";
        t.type = 42;
        t.value = 3443.34;
        list.append(t);
        t.name = "jfdjkkjfkjfg";
        t.type = 22;
        t.value = -133.224;
        list.append(t);
        QSettings s("test.ini", QSettings::IniFormat);
        s.setValue("test", QVariant::fromValue(list));
    }

    {
        QList<TestStructure> list;
        QSettings s("test.ini", QSettings::IniFormat);
        QVariant value = s.value("test");
        list = value.value<QList<TestStructure>>();
        foreach (const TestStructure &t, list)
            qDebug() << t.name << t.type << t.value;
    }

    return;*/
/*
    qDebug() << sizeof(transactions::Transaction);

    const int NUM = 10000;


    quint64 res = 0;

    for (int k = 0; k < 20; k++) {
        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        //std::list<transactions::Transaction> container;
        std::vector<transactions::Transaction> container;
        container.reserve(11000);
        for (int n = 0; n < NUM; n++) {
            transactions::Transaction t;
            t.address = QStringLiteral("ldkldfkldfklgfklgfklgfklgf");
            t.value = QStringLiteral("ldkldfkldfklgfklgfklgfklgf");
            t.from = QStringLiteral("ldkldfkldfklgfklgfklgfklgf");
            t.to = QStringLiteral("ldkldfkldfklgfklgfklgfklgf");
            t.fee = QStringLiteral("ldkldfkldfklgfklgfklgfklgf");
            container.push_back(t);
        }

        std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();

        //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count() <<std::endl;
        //std::cout << "Time difference = " << std::chrono::duration_cast<std::chrono::nanoseconds> (end - begin).count() <<std::endl;
        res += std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
    }
    qDebug() << res / 20;
    return;
    */
    /*
    HttpSimpleClient *client = new HttpSimpleClient();
    client->sendMessagePost(QUrl("http://www.google.com:80/jkjktrjkt"), "{\"jsonrpc\":\"2.0\",\"method\":\"mhc_send\",\"params\":{\"data\":\"\",\"fee\":\"\",\"nonce\":\"3\",\"pubkey\":\"3059301306072A8648CE3D020106082A8648CE3D030107034200047FAF41D8AC6C965467EC24A8B78E528892DFEBAA91EEE770443EA574F4A384D0C305ED6FE97A2652241C4F6E2EE7A2306354EF39AFAAE2C0156C7FF69ECE3110\",\"sign\":\"3044022035538e5d047d394b58cb3046d4bc387d456cf850bcfe5231244438e98740acf8022045dc2d1750689f820c43ef26a4058cdbcaa5e5dbd2e7e80115baf3639989587b\",\"to\":\"0x00caceded040cdbfcc6bc4f43a72133252f4cb402478356dff\",\"value\":\"1\"}}",
                            [](const std::string &response, const TypedException &exception) {
        LOG << "Ups " << response;
    });
    const QString message = "{\"jsonrpc\":\"2.0\",\"method\":\"mhc_send\",\"params\":{\"data\":\"\",\"fee\":\"\",\"nonce\":\"3\",\"pubkey\":\"3059301306072A8648CE3D020106082A8648CE3D030107034200047FAF41D8AC6C965467EC24A8B78E528892DFEBAA91EEE770443EA574F4A384D0C305ED6FE97A2652241C4F6E2EE7A2306354EF39AFAAE2C0156C7FF69ECE3110\",\"sign\":\"636c61737320436f6e7472616374200a7b20200a09636f6e7374727563746f722829200a097b200a0909746869732e6f776e6572203d206d73672e73656e6465723b0a0909746869732e6d696e56616c7565203d20303b0a0909746869732e616d6f756e74203d206d73672e76616c75653b0a0909746869732e7061796d656e744c697374203d206e6577204d61703b0a0909746869732e6e616d654c697374203d206e6577204d61703b0a097d200a097061792829200a097b200a0909696620286d73672e76616c7565203c20746869732e6d696e56616c7565290a09097b200a0909097468726f7720747275653b0a09097d0a0909656c73650a09097b200a0909096c657420637572416472657373416d6f756e74203d20746869732e7061796d656e744c6973742e676574286d73672e73656e646572293b0a0909096c6574206375724d736756616c7565203d206d73672e76616c75653b0a090909696628637572416472657373416d6f756e7420213d3d20756e646566696e656420262620747970656f6620637572416472657373416d6f756e74203d3d3d20276e756d6265709097b200a0909097468726f7720747275653b0a09097d0a0909656c73650a09097b200a0909096c657420637572416472657373416d6f756e74203d20746869732e7061796d656e744c6973742e676574286d73672e73656e646572293b0a0909096c6574206375724d736756616c7565203d206d73672e76616c75653b0a074203d3d3d20276e756d6265709097b200a0909097468726f7720747275653b0a09097d0a0909656c73650a09097b200a0909096c657420637572416472657373416d6f756e74203d20746869732e7061796d656e744c693d3d3d20276e76d6f756e74203d20746869732e7061796d656e744c6973742e676574286d73672e73656e646572293b0a0909096c6574206374206375724d736756616c7565203d206d73672e76616c75653b0a074203d3d3d20276e756d6265709097b200a0909097468726f7720747275653b0a09097d0a0909656c73650a09097b200a0909096c657420637572416472657373416d6f756e74203d20746869732e7061796d656e744c6973742e676574286d73672e73656e646572293b0a0909096c6574206375724d736756616c7565203d206d73672e76616c75653b0a090909696628637572416472657373416d6f756e7420213d3d20756e646566696e656420262620747970656f6620637572416472657373416d6f756e74203d3d3d20276e756d6265709097b200a0909097468726f7720747275653b0a09097d0a0909656c73650a09097b200a0909096c657420637572416472657373416d6f756e74203d20746869732e7061796d656e744c6973742e676574286d73672e73656e646572293b0a0909096c6574206375724d736756616c7565203d206d73672e76616c75653b0a090909696628637572416472657373416d6f756e7420213d3d20756e646566696e656420262620747970656f6620637572416472657373416d6f756e74203d3d3d20276e756d62657\",\"to\":\"0x00caceded040cdbfcc6bc4f43a72133252f4cb402478356dff\",\"value\":\"1\"}}";
    client->sendMessagePost(QUrl("http://188.246.233.140:9999/"), message,
                            [](const std::string &response, const TypedException &exception) {
        LOG << "Ups " << response;
    });

    HttpSocket *httpsocket = new HttpSocket(QUrl("http://188.246.233.140:9999/"), message);
    */
    if (QFile::exists("messenger.db"))
        QFile::remove("messenger.db");
    BEGIN_SLOT_WRAPPER
            messenger::MessengerDBStorage db;
    //db.openDB();
    db.init();

    db.setSettings("key1", "vaaa");
    db.setSettings("key1", "vaaa123");
    qDebug() << db.getSettings("key1");
    qDebug() << db.getSettings("key2");

    //    db.addPayment("818af8b3a50a5b6758360d11a44533f596af76feb9b4e57557220a8536c38165",
    //                                      "818af8b3a50a5b6758360d11a44533f596af76feb9b4e57557220a8536c38165",
    //                                      "1CKZ3fJrojYauQHUZiuSKeuvw1AwBVUwHX",
    //                                      "19BWPGPJM9KAG6r8vRsLH9e6w495dx3C5W,1",
    //                                      "0.0006650",
    //                                      "66502",
    //                                      533227,
    //                                      true,
    //                                      1532329669,
    //                                      "7");
    DBStorage::DbId id1 = db.getUserId("1234");
    db.addChannel(id1, "channel", "jkgfjkgfgfitrrtoioriojk", true, "ktkt", false, true, true);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1, "jkgfjkgfgfitrrtoioriojk");
    db.addMessage("1234", "3454", "abcd", 1, 4001, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4002, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4003, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4004, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 1500, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 6000, true, true, true, "asdfdf", 1, "jkgfjkgfgfitrrtoioriojk");

    messenger::MessengerDBStorage::IdCounterPair p = db.findFirstNotConfirmedMessageWithHash("1234", "asdfdf");
    qDebug() << "P " << p.second;
    p = db.findFirstMessageWithHash("1234", "asdfdf");
    qDebug() << "P " << p.second;

    p = db.findFirstNotConfirmedMessageWithHash("1234", "asdfdf", "jkgfjkgfgfitrrtoioriojk");
    qDebug() << "P " << p.second;
    p = db.findFirstMessageWithHash("1234", "asdfdf", "jkgfjkgfgfitrrtoioriojk");
    qDebug() << "P " << p.second;

    qDebug() << db.getMessageMaxCounter("1234");
    qDebug() << db.getMessageMaxCounter("1234", "jkgfjkgfgfitrrtoioriojk");
    qDebug() << db.hasMessageWithCounter("1234", 4000);
    qDebug() << db.hasMessageWithCounter("1234", 4001);
    qDebug() << db.hasMessageWithCounter("1234", 4000, "jkgfjkgfgfitrrtoioriojk");
    qDebug() << db.hasMessageWithCounter("1234", 4001, "jkgfjkgfgfitrrtoioriojk");
    //qDebug() << db.getMessageMaxConfirmedCounter("user7");
    //qDebug() << db.getMessageMaxConfirmedCounter("userururut");




    qDebug() << "answer " << db.getMessagesForUserAndDestNum("1234", "3454", 5000, 20, false).size();
    qDebug() << "answer " << db.getMessagesCountForUserAndDest("1234", "3454", 3000);
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

    qDebug() << "size" << db.getMessagesForUserAndDestNum("user7", "user1", 10, 1000).size();

    std::vector<messenger::Message> msgs = db.getMessagesForUser("user7", 1, 3);
    qDebug() << "count " << msgs.size();

    qDebug() << db.getMessageMaxCounter("user7");
    qDebug() << db.getMessageMaxConfirmedCounter("user7");
    qDebug() << db.getMessageMaxConfirmedCounter("userururut");

    qDebug() << db.getUsersList();

    db.setUserPublicKey("user7", "dfkgflgfkltrioidfkldfklgfgf");
    qDebug() << db.getUserPublicKey("user7");
    qDebug() << db.getUserPublicKey("user1");
    qDebug() << db.getUserPublicKey("userrrrr");
    qint64 id = db.findFirstNotConfirmedMessage("user7");
    qDebug() << id;
    db.updateMessage(id, 4445, true);
    qDebug() << db.findFirstNotConfirmedMessage("user7");


    qDebug() << db.getLastReadCounterForUserContact("userrgjkg", "fjkgfjk");
    qDebug() << db.getLastReadCounterForUserContact("user7", "user1");
    db.setLastReadCounterForUserContact("user7", "user1", 244);
    qDebug() << db.getLastReadCounterForUserContact("user7", "user1");


    qDebug() << db.getLastReadCountersForContacts("user7").size();


    db.addChannel(1, "chnl1", "jkgfjkgfgfioioriojk", true, "ktkt", false, true, true);
    db.addChannel(1, "chnl2", "jkgfjkgfgfioioriojk", false, "ktkt", false, true, true);
    db.addChannel(2, "chnl1", "jkgfjkgfgfioioriojk", true, "kfgfgft", false, true, true);


    db.setChannelsNotVisited("user1");
    qDebug() << db.getChannelForUserShaName("1234", "jkgfjkgfgfioioriojk");
    db.updateChannel(1, false);
    db.setWriterForNotVisited("1234");


    db.getChannelInfoForUserShaName("1234", "jkgfjkgfgfioioriojk");
    db.setChannelIsWriterForUserShaName("1234", "jkgfjkgfgfioioriojk", true);

    return;
    transactions::TransactionsDBStorage tdb;
    tdb.init();

    tdb.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", true, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    tdb.addPayment("mh", "gfklklkltrklklklgfkfhg", "address100", true, "user7", "user2", "1334", 568869454456, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    tdb.addPayment("mh", "gfklklkltjjkguieriufhg", "address100", true, "user7", "user1", "100", 568869445334, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    tdb.addPayment("mh", "gfklklklruuiuiduidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    qDebug() << tdb.getPaymentsForAddress("address", "mh", 0, 3, true).size();
    /*

    //9223372036854775807
    //100000000000000000000
    for (int n = 0; n < 1000; n++) {

        tdb.addPayment("mh", "gfklklkltrklklklgfkfhg", true, "user2", "user3", "100000000000000000", 5688694544 + 2 * n, "nvcmnjkdfjkgf", 100, 8896865);
    }*/

    /*for (int n = 0; n < 100; n++) {

        tdb.addPayment("mh", "gfklklkltrklklklgfkfhg", false, "user2", "user3", "200000000000000001", 5688694543 + 2 * n, "nvcmnjkdfjkgf", 100, 8896865);
    }*/

    qDebug() << tdb.getPaymentsCountForAddress("address100", "mh", true);
    qDebug() << tdb.getPaymentsCountForAddress("address100", "mh", false);
    BigNumber res1, res2;
    res1 = tdb.calcInValueForAddress("address100", "mh");
    res2 = tdb.calcOutValueForAddress("address100", "mh");
    //res1 = res1 - res2;


    tdb.addTracked("mh", "address101", "namew", "typeee", "grp333");
    tdb.addTracked("mh", "address103", "namew", "typeee", "grp333");
    tdb.addTracked("mh", "address100", "namew", "typeee", "grp333");
    tdb.addTracked("mh", "address100", "namew", "typeee", "grp331");

    qDebug() << tdb.getTrackedForGroup("grp333").size();

    qDebug() << res1.getDecimal();
    qDebug() << res2.getDecimal();
    BigNumber num1(QByteArray("1111111111111111111111111111111111111111111111"));
    BigNumber num2(QByteArray("2222222222222222222222222222222222222222222222"));
    //num1 += num2;

    qDebug() << num1.getDecimal();
    qDebug() << num2.getDecimal();
    BigNumber r = num2 - num1;
    r = r - num1;
    r = r - num1;
    qDebug() << r.getDecimal();
    END_SLOT_WRAPPER

            //    QTcpSocket
            //        SimpleClient client;
            //        client.sendMessagePost(QUrl("http://188.246.233.140:9999/"), "{\"jsonrpc\":\"2.0\",\"method\":\"mhc_send\",\"params\":{\"data\":\"\",\"fee\":\"\",\"nonce\":\"3\",\"pubkey\":\"3059301306072A8648CE3D020106082A8648CE3D030107034200047FAF41D8AC6C965467EC24A8B78E528892DFEBAA91EEE770443EA574F4A384D0C305ED6FE97A2652241C4F6E2EE7A2306354EF39AFAAE2C0156C7FF69ECE3110\",\"sign\":\"3044022035538e5d047d394b58cb3046d4bc387d456cf850bcfe5231244438e98740acf8022045dc2d1750689f820c43ef26a4058cdbcaa5e5dbd2e7e80115baf3639989587b\",\"to\":\"0x00caceded040cdbfcc6bc4f43a72133252f4cb402478356dff\",\"value\":\"1\"}}", [](const std::string &response) {
            //            LOG << "Ups";
            //        });

}

MainWindow::~MainWindow()
{
    delete ui;
}

QDataStream &operator<<(QDataStream &out, const TestStructure &obj)
{
    out << obj.currency
        << obj.address
        << obj.type
        << obj.group
        << obj.name
        << obj.value;

    return out;
}

QDataStream &operator>>(QDataStream &in, TestStructure &obj)
{
    in >> obj.currency
        >> obj.address
        >> obj.type
        >> obj.group
        >> obj.name
        >> obj.value;
    return in;
}
