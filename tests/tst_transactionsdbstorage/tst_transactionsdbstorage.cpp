#include "tst_transactionsdbstorage.h"

#include "SlotWrapper.h"

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
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", true, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltrklklklgfkfhg", "address100", true, "user7", "user2", "1334", 568869454456, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltjjkguieriufhg", "address100", true, "user7", "user1", "100", 568869445334, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklkl545uuiuiduidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1004040", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklklrttrrrduidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "15434900", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklklruuiuifdidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1435400", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklklrddfgiduidgjkg", "address100", false, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1054030", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gtrgklklrddfgiduidgjkg", "address100", true, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1334430", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklklti5o0rruidgjkg", "address100", true, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1069590", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh2", "gfklklklti5o0rruidgjkg", "address100", true, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1069590", "jkgh", transactions::Transaction::OK);

    db.addPayment("mh", "gfklklkltrkjtrtritrdf1", "address100", true, "user7", "user2", "1334", 568869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "100", "jkgh", transactions::Transaction::PENDING);
    db.addPayment("mh", "wuklklkltrkjtrtritrdf1", "address100", true, "user7", "user2", "1334", 564869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "100", "jkgh", transactions::Transaction::PENDING);
    db.addPayment("mh", "fkfkgkgktrkjtrtritrdf1", "address100", true, "user7", "user2", "1334", 545869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "100", "jkgh", transactions::Transaction::PENDING);

    db.addPayment("mh", "gfklklkltrkjtrtritrdf12", "address100", true, "user7", "user2", "1334", 568869453456, "nvcmnjkdfjkgf", "100", 8896865, true, true, "100", "jkgh", transactions::Transaction::PENDING);
    db.addPayment("mh", "wuklklkltrе1tritrdf11", "address100", true, "user7", "user2", "1334", 564869453456, "nvcmnjkdfjkgf", "100", 8896865, true, true, "100", "jkgh", transactions::Transaction::PENDING);

    db.addPayment("mh", "gfklklkltrkjtrtritrdf134", "address100", false, "user7", "user2", "1334", 568869453456, "nvcmnjkdfjkgf", "100", 8896865, true, true, "33", "jkgh", transactions::Transaction::PENDING);
    db.addPayment("mh", "wuklklkltrkjtrtritrdf215", "address100", false, "user7", "user2", "1334", 564869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1", "jkgh", transactions::Transaction::PENDING);
    db.addPayment("mh", "fkfkgkgktrkjtrtritrdf611", "address100", false, "user7", "user2", "1334", 545869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "100", "jkgh", transactions::Transaction::PENDING);


    BigNumber ires = db.calcInValueForAddress("address100", "mh");
    BigNumber ores = db.calcOutValueForAddress("address100", "mh");
    QCOMPARE(ires.getDecimal(), QByteArray("14784"));
    QCOMPARE(ores.getDecimal(), QByteArray("13362"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", true, false).getDecimal(), QByteArray("16870300"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", false, false).getDecimal(), QByteArray("2058070"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", true, true).getDecimal(), QByteArray("1069590"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", false, true).getDecimal(), QByteArray("1334430"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", true, false, transactions::Transaction::PENDING).getDecimal(), QByteArray("33"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", false, false, transactions::Transaction::PENDING).getDecimal(), QByteArray("101"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", true, true, transactions::Transaction::PENDING).getDecimal(), QByteArray("200"));
    QCOMPARE(db.calcIsSetDelegateValueForAddress("address100", "mh", false, true, transactions::Transaction::PENDING).getDecimal(), QByteArray("300"));
    QCOMPARE(db.getIsSetDelegatePaymentsCountForAddress("address100", "mh"), 6);

    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh", true), 10);
    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh", false), 7);
    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh2", false), 0);

    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", true, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh", true), 10);

    std::vector<transactions::Transaction> res = db.getPaymentsForAddressPending("address100", "mh", true);
    transactions::Transaction trans = res.at(0);
    QCOMPARE(res.size(), 8);
    QCOMPARE(res.at(0).address, QStringLiteral("address100"));
    QCOMPARE(res.at(0).tx, QStringLiteral("fkfkgkgktrkjtrtritrdf1"));
    QCOMPARE(res.at(0).currency, QStringLiteral("mh"));
    QCOMPARE(res.at(0).isInput, true);
    QCOMPARE(res.at(0).from, QStringLiteral("user7"));
    QCOMPARE(res.at(0).to, QStringLiteral("user2"));
    QCOMPARE(res.at(0).value, QStringLiteral("1334"));
    QCOMPARE(res.at(0).timestamp, 545869453456);
    QCOMPARE(res.at(0).data, QStringLiteral("nvcmnjkdfjkgf"));
    QCOMPARE(res.at(0).fee, QStringLiteral("100"));
    QCOMPARE(res.at(0).nonce, 8896865);
    QCOMPARE(res.at(0).isDelegate, false);
    QCOMPARE(res.at(0).isSetDelegate, true);
    QCOMPARE(res.at(0).delegateValue, QStringLiteral("100"));
    QCOMPARE(res.at(0).status, transactions::Transaction::PENDING);
    QCOMPARE(res.at(0).delegateHash, QStringLiteral("jkgh"));


    trans.from = "a1";
    trans.to = "a2";
    trans.value = "a3";
    trans.timestamp = 1;
    trans.data = "a5";
    trans.fee = "a6";
    trans.nonce = 7;
    trans.isSetDelegate = false;
    trans.isDelegate = true;
    trans.delegateValue = "a8";
    trans.delegateHash = "a9";
    trans.status = transactions::Transaction::ERROR;
    db.updatePayment("address100", "mh", "fkfkgkgktrkjtrtritrdf1", true, trans);


    res = db.getPaymentsForAddressPending("address100", "mh", true);
    QCOMPARE(res.size(), 7);


    res = db.getPaymentsForAddress("address100", "mh", 0, 2, true);
    trans = res.at(0);
    QCOMPARE(trans.address, QStringLiteral("address100"));
    QCOMPARE(trans.tx, QStringLiteral("fkfkgkgktrkjtrtritrdf1"));
    QCOMPARE(trans.currency, QStringLiteral("mh"));
    QCOMPARE(trans.isInput, true);
    QCOMPARE(trans.from, QStringLiteral("a1"));
    QCOMPARE(trans.to, QStringLiteral("a2"));
    QCOMPARE(trans.value, QStringLiteral("a3"));
    QCOMPARE(trans.timestamp, 1);
    QCOMPARE(trans.data, QStringLiteral("a5"));
    QCOMPARE(trans.fee, QStringLiteral("a6"));
    QCOMPARE(trans.nonce, 7);
    QCOMPARE(trans.isDelegate, true);
    QCOMPARE(trans.isSetDelegate, false);
    QCOMPARE(trans.delegateValue, QStringLiteral("a8"));
    QCOMPARE(trans.status, transactions::Transaction::ERROR);
    QCOMPARE(trans.delegateHash, QStringLiteral("a9"));

    qint64 count = db.getPaymentsCountForAddress("address100", "mh", true);
    QCOMPARE(count, 10);

    db.removePaymentsForCurrency("mh");
    res = db.getPaymentsForAddressPending("address100", "mh", true);
    QCOMPARE(res.size(), 0);
    count = db.getPaymentsCountForAddress("address100", "mh", true);
    QCOMPARE(count, 0);
}

void tst_TransactionsDBStorage::testBigNumSum()
{
    if (QFile::exists(dbName))
        QFile::remove(dbName);
    transactions::TransactionsDBStorage db;
    db.init();
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", true, "user7", "user1", "9000000000000000000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltrkgklgfmjgfhg", "address100", true, "user7", "user1", "9000000000000000000", 568869455887, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltrklblgfmjgfhg", "address100", true, "user7", "user1", "9000000000000000000", 568869455888, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltrklklgssjgfhg", "address100", true, "user7", "user1", "9000000000000000000", 568869455889, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltrkgklgfmjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455887, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltrklblgfmjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455888, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
    db.addPayment("mh", "gfklklkltrklklgssjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455889, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);

    db.addPayment("mh", "gfklklkltrklklgssjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455889, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK);
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
    auto transactionGuard = db.beginTransaction();
    for (int n = 0; n < 100; n++) {
        db.addPayment("mh", QString("gfklklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address100", true, "user7", "user1", "9000000000000000000", 1000 + 2 * n, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "kghkghk", transactions::Transaction::OK);
        db.addPayment("mh", QString("ggrlklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address20", true, "user7", "user1", "1000000000000000000", 1000 + 2 * n + 1, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "gffkl", transactions::Transaction::OK);
    }
    transactionGuard.commit();
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

void tst_TransactionsDBStorage::testAddressInfos()
{
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
