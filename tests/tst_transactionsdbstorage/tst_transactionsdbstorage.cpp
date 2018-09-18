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
    db.addPayment("mh", "gfklkl545uuiuiduidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1004040");
    db.addPayment("mh", "gfklklklrttrrrduidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "15434900");
    db.addPayment("mh", "gfklklklruuiuifdidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1435400");
    db.addPayment("mh", "gfklklklrddfgiduidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1054030");
    db.addPayment("mh", "gtrgklklrddfgiduidgjkg", "address100", true, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1334430");
    db.addPayment("mh", "gfklklklti5o0rruidgjkg", "address100", true, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1069590");
    db.addPayment("mh2", "gfklklklti5o0rruidgjkg", "address100", true, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1069590");


    BigNumber ires = db.calcInValueForAddress("address100", "mh");
    BigNumber ores = db.calcOutValueForAddress("address100", "mh");
    QCOMPARE(ires.getDecimal(), QByteArray("7614"));
    QCOMPARE(ores.getDecimal(), QByteArray("9360"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", true, false).getDecimal(), QByteArray("16870300"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", false, false).getDecimal(), QByteArray("2058070"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", true, true).getDecimal(), QByteArray("1069590"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", false, true).getDecimal(), QByteArray("1334430"));
    QCOMPARE(db.getIsSetDelegatePaymentsCountForAddress("address100", "mh"), 6);

    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh", true), 5);
    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh", false), 4);
    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh2", false), 0);

    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", true, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh", true), 5);
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

    res = db.getPaymentsForCurrency("mh", 55, 10, true);

    r = 0;
    for (auto it = res.begin(); it != res.end (); ++it) {
        QCOMPARE(it->timestamp, 1000 + 55 + r);
        QCOMPARE(it->currency, QStringLiteral("mh"));
        if (r % 2)
            QCOMPARE(it->address, QStringLiteral("address100"));
        else
            QCOMPARE(it->address, QStringLiteral("address20"));
        r++;
    }
}

void tst_TransactionsDBStorage::testAddressInfos() {
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    transactions::TransactionsDBStorage db;
    db.init();

    db.addTracked(transactions::AddressInfo("mh", "address1", "type1", "group1", "name1"));
    db.addTracked(transactions::AddressInfo("mh", "address1", "type1", "group1", "name1"));
    db.addTracked(transactions::AddressInfo("mh2", "address2", "type2", "group2", "name2"));
    db.addTracked(transactions::AddressInfo("mh3", "address3", "type3", "group1", "name3"));

    const std::vector<transactions::AddressInfo> trackeds = db.getTrackedForGroup("group1");
    QCOMPARE(trackeds.size(), 2);
    for (const transactions::AddressInfo &info: trackeds) {
        if (info.address == "address1") {
            QCOMPARE(info.address, "address1");
            QCOMPARE(info.type, "type1");
            QCOMPARE(info.group, "group1");
            QCOMPARE(info.name, "name1");
            QCOMPARE(info.currency, "mh");
        } else {
            QCOMPARE(info.address, "address3");
            QCOMPARE(info.type, "type3");
            QCOMPARE(info.group, "group1");
            QCOMPARE(info.name, "name3");
            QCOMPARE(info.currency, "mh3");
        }
    }
}

QTEST_MAIN(tst_TransactionsDBStorage)
