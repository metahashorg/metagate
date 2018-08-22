#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTreeWidget>
#include "dbres.h"
#include "dbstorage.h"
#include "messengerdbstorage.h"
#include "transactionsdbstorage.h"
#include "SlotWrapper.h"
#include "BigNumber.h"

#include <QDebug>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
BEGIN_SLOT_WRAPPER
    MessengerDBStorage db;
    db.openDB();
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
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 4000, true, true, true, "asdfdf", 1);
    db.addMessage("1234", "3454", "abcd", 1, 1500, true, true, true, "asdfdf", 1);
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

    qDebug() << "size" << db.getMessagesForUserAndDestNum("user7", "user1", 10, 1000).size();

    std::list<Message> msgs = db.getMessagesForUser("user7", 1, 3);
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


    qDebug() << db.getLastReadCountersForUser("user7").size();


    db.addChannel(1, "chnl1", "jkgfjkgfgfioioriojk", true, "ktkt", false, true, true);
    db.addChannel(1, "chnl2", "jkgfjkgfgfioioriojk", false, "ktkt", false, true, true);
    db.addChannel(2, "chnl1", "jkgfjkgfgfioioriojk", true, "kfgfgft", false, true, true);


    db.setChannelsNotVisited("user1");
    qDebug() << db.getChannelForUserShaName("1234", "jkgfjkgfgfioioriojk");
    db.updateChannel(1, false);
    db.setWriterForNotVisited("1234");


    db.getChannelInfoForUserShaName("1234", "jkgfjkgfgfioioriojk");
    db.setChannelIsWriterForUserShaName("1234", "jkgfjkgfgfioioriojk", true);

    transactions::TransactionsDBStorage tdb;
    tdb.init();

  /*  tdb.addPayment("mh", "gfklklkltrklklgfmjgfhg", true, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", 100, 8896865);
    tdb.addPayment("mh", "gfklklkltrklklklgfkfhg", true, "user7", "user2", "1334", 568869454456, "nvcmnjkdfjkgf", 100, 8896865);
    tdb.addPayment("mh", "gfklklkltjjkguieriufhg", true, "user7", "user1", "100", 568869445334, "nvcmnjkdfjkgf", 100, 8896865);
    tdb.addPayment("mh", "gfklklklruuiuiduidgjkg", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", 100, 8896865);
    qDebug() << tdb.getPaymentsForDest("user7", "user1", "mh", 0, 3, true).size();


    //9223372036854775807
    //100000000000000000000
    for (int n = 0; n < 1000; n++) {

        tdb.addPayment("mh", "gfklklkltrklklklgfkfhg", true, "user2", "user3", "100000000000000000", 5688694544 + 2 * n, "nvcmnjkdfjkgf", 100, 8896865);
    }*/

    /*for (int n = 0; n < 100; n++) {

        tdb.addPayment("mh", "gfklklkltrklklklgfkfhg", false, "user2", "user3", "200000000000000001", 5688694543 + 2 * n, "nvcmnjkdfjkgf", 100, 8896865);
    }*/

    qDebug() << tdb.getPaymentsCountForDest("user2", "user3", "mh", true);
    qDebug() << tdb.getPaymentsCountForDest("user2", "user3", "mh", false);
    BigNumber res1, res2;
    res1 = tdb.calcInValueForDest("user2", "user3", "mh");
    res2 = tdb.calcOutValueForDest("user2", "user3", "mh");
    //res1 = res1 - res2;


    tdb.addTracked("mh", "user7", "user1", "typeee", "grp333");
    tdb.addTracked("mh", "user7", "user1", "typeee", "grp331");

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
}

MainWindow::~MainWindow()
{
    delete ui;
}
