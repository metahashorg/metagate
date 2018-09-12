#include "tst_transactionsdbstorage.h"

const QString dbName = "payments.db";

tst_TransactionsDBStorage::tst_TransactionsDBStorage(QObject *parent)
    : QObject(parent)
{
}

void tst_TransactionsDBStorage::testDB1()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    transactions::TransactionsDBStorage db;
    db.init();
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", true, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltrklklklgfkfhg", "address100", true, "user7", "user2", "1334", 568869454456, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltjjkguieriufhg", "address100", true, "user7", "user1", "100", 568869445334, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklklruuiuiduidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");

//    DBStorage::DbId id1 = db.getUserId("ddfjgjgj");
//    DBStorage::DbId id2 = db.getUserId("ddfjgjgj");
//    QCOMPARE(id1, id2);
}

void tst_TransactionsDBStorage::testBigNumSum()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    transactions::TransactionsDBStorage db;
    db.init();
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", true, "user7", "user1", "9000000000000000000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltrkgklgfmjgfhg", "address100", true, "user7", "user1", "9000000000000000000", 568869455887, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltrklblgfmjgfhg", "address100", true, "user7", "user1", "9000000000000000000", 568869455888, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltrklklgssjgfhg", "address100", true, "user7", "user1", "9000000000000000000", 568869455889, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltrkgklgfmjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455887, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltrklblgfmjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455888, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    db.addPayment("mh", "gfklklkltrklklgssjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455889, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    BigNumber ires = db.calcInValueForAddress("address100", "mh");
    BigNumber ores = db.calcOutValueForAddress("address100", "mh");
    QCOMPARE(ires.getDecimal(), QByteArray("36000000000000000400"));
    QCOMPARE(ores.getDecimal(), QByteArray("36000000000000000000"));

}

void tst_TransactionsDBStorage::testGetPayments()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    transactions::TransactionsDBStorage db;
    db.init();
    for (int n = 0; n < 100; n++) {
        db.addPayment("mh", QString("gfklklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address100", true, "user7", "user1", "9000000000000000000", 1000 + 2 * n, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
        db.addPayment("mh", QString("ggrlklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address20", true, "user7", "user1", "1000000000000000000", 1000 + 2 * n + 1, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    }
    qint64 count = db.getPaymentsCountForAddress("address100", "mh", true);
    QCOMPARE(count, 100);
    std::vector<transactions::Transaction> res = db.getPaymentsForAddress("address100", "mh", 55, 10, true);

    int r = 0;
    for (auto it = res.begin(); it != res.end (); ++it) {
        QCOMPARE(it->timestamp, 1110 + 2 * r);
        QCOMPARE(it->currency, QStringLiteral("mh"));
        QCOMPARE(it->address, QStringLiteral("address100"));
        r++;
    }


    res = db.getPaymentsForCurrency("mh", 55, 10, false);

    r = 0;
    for (auto it = res.begin(); it != res.end (); ++it) {
        QCOMPARE(it->timestamp, 1144 - r);
        QCOMPARE(it->currency, QStringLiteral("mh"));
        if (r % 2)
            QCOMPARE(it->address, QStringLiteral("address20"));
        else
            QCOMPARE(it->address, QStringLiteral("address100"));
        r++;
    }
    qDebug() << db.dbName();
}


QTEST_MAIN(tst_TransactionsDBStorage)
