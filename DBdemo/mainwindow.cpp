#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTreeWidget>
#include "dbres.h"
#include "dbstorage.h"
#include "messengerdbstorage.h"

#include <QDebug>
MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

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
    db.updateMessage(id, 4444, true);
    qDebug() << db.findFirstNotConfirmedMessage("user7");


    qDebug() << db.getLastReadCounterForUserContact("userrgjkg", "fjkgfjk");
    qDebug() << db.getLastReadCounterForUserContact("user7", "user1");
    db.setLastReadCounterForUserContact("user7", "user1", 244);
    qDebug() << db.getLastReadCounterForUserContact("user7", "user1");


    qDebug() << db.getLastReadCountersForUser("user7").size();
//    QList<QStringList> r = db.getPayments();
//    foreach(const QStringList &l, r) {
//        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
//        for (int c = 0; c < 10; c++)
//            item->setText(c, l.value(c));
//    }

}

MainWindow::~MainWindow()
{
    delete ui;
}
