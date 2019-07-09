#include "tst_transactionsdbstorage.h"

#include <QTest>

#include "TransactionsDBStorage.h"
#include "TransactionsDBRes.h"

tst_TransactionsDBStorage::tst_TransactionsDBStorage(QObject *parent)
    : QObject(parent)
{
}

void tst_TransactionsDBStorage::testDB1()
{
    if (QFile::exists(transactions::databaseFileName))
        QFile::remove(transactions::databaseFileName);
    transactions::TransactionsDBStorage db;
    db.init();
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", 0, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrklklklgfkfhg", "address100", 0, "user7", "user2", "1334", 568869454456, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11113, "3242", 2);
    db.addPayment("mh", "gfklklkltjjkguieriufhg", "address100", 1, "user7", "user1", "100", 568869445334, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11114, "", 1);
    db.addPayment("mh", "gfklkl545uuiuiduidgjkg", "address100", 2, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1004040", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11115, "324521354", 2);
    db.addPayment("mh", "gfklklklrttrrrduidgjkg", "address100", 3, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "15434900", "jkgh", transactions::Transaction::OK, transactions::Transaction::FORGING, 11116, "", 1);
    db.addPayment("mh", "gfklklklruuiuifdidgjkg", "address100", 4, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1435400", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11117, "", 1);
    db.addPayment("mh", "gfklklklrddfgiduidgjkg", "address100", 4, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1054030", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11118, "", 1);
    db.addPayment("mh", "gtrgklklrddfgiduidgjkg", "address100", 5, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1334430", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11119, "", 1);
    db.addPayment("mh", "gfklklklti5o0rruidgjkg", "address100", 4, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1069590", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh2", "gfklklklti5o0rruidgjkg", "address100", 4, "user7", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1069590", "jkgh", transactions::Transaction::OK, transactions::Transaction::FORGING, 111142, "", 1);

    db.addPayment("mh", "gfklklkltrkjtrtritrdf1", "address100", 3, "user7", "user2", "1334", 568869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "100", "jkgh", transactions::Transaction::PENDING, transactions::Transaction::SIMPLE, 111141, "34543", 1);
    db.addPayment("mh", "wuklklkltrkjtrtritrdf1", "address100", 4, "user7", "user2", "1334", 564869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "100", "jkgh", transactions::Transaction::PENDING, transactions::Transaction::SIMPLE, 111122, "34243", 1);
    db.addPayment("mh", "fkfkgkgktrkjtrtritrdf1", "address100", 2, "user7", "user2", "1334", 545869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "100", "jkgh", transactions::Transaction::PENDING, transactions::Transaction::SIMPLE, 111112, "", 1);

    db.addPayment("mh", "gfklklkltrkjtrtritrdf12", "address100", 3, "user7", "user2", "1334", 568869453456, "nvcmnjkdfjkgf", "100", 8896865, true, true, "100", "jkgh", transactions::Transaction::PENDING, transactions::Transaction::SIMPLE, 1111222, "2345324", 1);
    db.addPayment("mh", "wuklklkltrе1tritrdf11", "address100", 4, "user7", "user2", "1334", 564869453456, "nvcmnjkdfjkgf", "100", 8896865, true, true, "100", "jkgh", transactions::Transaction::PENDING, transactions::Transaction::SIMPLE, 111120, "", 1);

    db.addPayment("mh", "gfklklkltrkjtrtritrdf134", "address100", 5, "user7", "user2", "1334", 568869453456, "nvcmnjkdfjkgf", "100", 8896865, true, true, "33", "jkgh", transactions::Transaction::PENDING, transactions::Transaction::FORGING, 12332, "3453", 1);
    db.addPayment("mh", "wuklklkltrkjtrtritrdf215", "address100", 5, "user7", "user2", "1334", 564869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1", "jkgh", transactions::Transaction::PENDING, transactions::Transaction::SIMPLE, 11232, "", 1);
    db.addPayment("mh", "fkfkgkgktrkjtrtritrdf611", "address100", 6, "user7", "user2", "1334", 545869453456, "nvcmnjkdfjkgf", "100", 8896865, true, false, "100", "jkgh", transactions::Transaction::PENDING, transactions::Transaction::SIMPLE, 11455, "", 1);
    db.addPayment("mh3", "gfklklklti5o0rruidgjkg", "address100", 7, "address100", "address100", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1069590", "jkgh", transactions::Transaction::OK, transactions::Transaction::FORGING, 111142, "", 1);
    db.addPayment("mh3", "gfklklklti5o0rruidgjkg", "address100", 7, "address100", "address100", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1069590", "jkgh", transactions::Transaction::OK, transactions::Transaction::FORGING, 111142, "", 1);
    {
        const transactions::Transaction tx1 = db.getLastTransaction("address100", "mh");
        QCOMPARE(tx1.blockNumber, 1111222);
        QCOMPARE(tx1.blockHash, "2345324");
        const transactions::Transaction tx3 = db.getLastTransaction("address10", "mh");
        QCOMPARE(tx3.blockNumber, 0);
    }

    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh"), 17);
    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh2"), 1);
    QCOMPARE(db.getPaymentsCountForAddress("address100", "mh3"), 1);

    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", 4, "user7", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);

    std::vector<transactions::Transaction> res = db.getPaymentsForAddressPending("address100", "mh", true);
    transactions::Transaction trans = res.at(0);
    QCOMPARE(res.size(), 8);
    QCOMPARE(res.at(0).address, QStringLiteral("address100"));
    QCOMPARE(res.at(0).tx, QStringLiteral("fkfkgkgktrkjtrtritrdf1"));
    QCOMPARE(res.at(0).currency, QStringLiteral("mh"));
    QCOMPARE(res.at(0).from, QStringLiteral("user7"));
    QCOMPARE(res.at(0).to, QStringLiteral("user2"));
    QCOMPARE(res.at(0).value, QStringLiteral("1334"));
    QCOMPARE(res.at(0).timestamp, 545869453456);
    QCOMPARE(res.at(0).data, QStringLiteral("nvcmnjkdfjkgf"));
    QCOMPARE(res.at(0).fee, QStringLiteral("100"));
    QCOMPARE(res.at(0).nonce, 8896865);
    QCOMPARE(res.at(0).blockIndex, 2);
    QCOMPARE(res.at(0).isDelegate, false);
    QCOMPARE(res.at(0).isSetDelegate, true);
    QCOMPARE(res.at(0).delegateValue, QStringLiteral("100"));
    QCOMPARE(res.at(0).status, transactions::Transaction::PENDING);
    QCOMPARE(res.at(0).delegateHash, QStringLiteral("jkgh"));

    res = db.getForgingPaymentsForAddress("address100", "mh", 0, -1, true);
    QCOMPARE(res.size(), 2);

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
    trans.type = transactions::Transaction::FORGING;
    trans.blockIndex = 2;
    trans.blockNumber = 111112;
    db.updatePayment("address100", "mh", "fkfkgkgktrkjtrtritrdf1", trans.blockNumber, trans.blockIndex, trans);


    res = db.getPaymentsForAddressPending("address100", "mh", true);
    QCOMPARE(res.size(), 7);


    res = db.getPaymentsForAddress("address100", "mh", 0, 2, true);
    trans = res.at(0);
    QCOMPARE(trans.address, QStringLiteral("address100"));
    QCOMPARE(trans.tx, QStringLiteral("fkfkgkgktrkjtrtritrdf1"));
    QCOMPARE(trans.currency, QStringLiteral("mh"));
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
    QCOMPARE(trans.type, transactions::Transaction::FORGING);
    QCOMPARE(trans.blockNumber, 111112);

    res = db.getForgingPaymentsForAddress("address100", "mh", 0, -1, true);
    QCOMPARE(res.size(), 3);

    trans = db.getLastForgingTransaction(QStringLiteral("address100"), QStringLiteral("mh"));
    QCOMPARE(trans.address, QStringLiteral("address100"));
    QCOMPARE(trans.tx, QStringLiteral("gfklklklrttrrrduidgjkg"));
    QCOMPARE(trans.currency, QStringLiteral("mh"));
    QCOMPARE(trans.from, QStringLiteral("user7"));
    QCOMPARE(trans.to, QStringLiteral("user3"));
    QCOMPARE(trans.value, QStringLiteral("2340"));
    QCOMPARE(trans.timestamp, 568869455856);
    QCOMPARE(trans.data, QStringLiteral("nvcmnjkdfjkgf"));
    QCOMPARE(trans.fee, QStringLiteral("100"));
    QCOMPARE(trans.nonce, 8896865);
    QCOMPARE(trans.isDelegate, true);
    QCOMPARE(trans.isSetDelegate, true);
    QCOMPARE(trans.delegateValue, QStringLiteral("15434900"));
    QCOMPARE(trans.status, transactions::Transaction::OK);
    QCOMPARE(trans.delegateHash, QStringLiteral("jkgh"));
    QCOMPARE(trans.type, transactions::Transaction::FORGING);
    QCOMPARE(trans.blockNumber, 11116);


    db.removePaymentsForCurrency("mh");
    res = db.getPaymentsForAddressPending("address100", "mh", true);
    QCOMPARE(res.size(), 0);
}

void tst_TransactionsDBStorage::tstFilterDelegate() {
    if (QFile::exists(transactions::databaseFileName))
        QFile::remove(transactions::databaseFileName);
    transactions::TransactionsDBStorage db;
    db.init();
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", 11, "address100", "user1", "1000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrklklklgfkfhg", "address100", 11, "address100", "user1", "1334", 568869454456, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11113, "3242", 2);
    db.addPayment("mh", "gfklklkltjjkguieriufhg", "address100", 11, "address100", "user1", "100", 568869445334, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::DELEGATE, 11114, "", 1);
    db.addPayment("mh", "gfklkl545uuiuiduidgjkg", "address100", 2, "address100", "user1", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1004040", "jkgh", transactions::Transaction::OK, transactions::Transaction::DELEGATE, 11115, "324521354", 2);
    db.addPayment("mh", "gfklklklrttrrrduidgjkg", "address100", 2, "user7", "user1", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "15434900", "jkgh", transactions::Transaction::OK, transactions::Transaction::FORGING, 11116, "", 1);
    db.addPayment("mh", "gfklklklruuiuifdidgjkg", "address100", 2, "address100", "user1", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1435400", "jkgh", transactions::Transaction::ERROR, transactions::Transaction::DELEGATE, 11117, "", 1);
    db.addPayment("mh", "gfklklklrddfgiduidgjkg", "address100", 2, "user1", "user3", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1054030", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11118, "", 1);
    db.addPayment("mh", "gfklklklruuiuifdidgjkg", "address100", 2, "user1", "address100", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, true, "1435400", "jkgh", transactions::Transaction::OK, transactions::Transaction::DELEGATE, 11119, "", 1);
    db.addPayment("mh", "gfklkl545uuiuiduidgjkg", "address100", 2, "address100", "user2", "2340", 568869455856, "nvcmnjkdfjkgf", "100", 8896865, true, false, "1004040", "jkgh", transactions::Transaction::OK, transactions::Transaction::DELEGATE, 11120, "324521354", 2);

    const auto res = db.getDelegatePaymentsForAddress("address100", "user1", "mh", 0, -1, true);
    QCOMPARE(res.size(), 2);

    const auto res2 = db.getDelegatePaymentsForAddress("address100", "mh", 0, -1, true);
    QCOMPARE(res2.size(), 3);
}

static void compareBalances(const transactions::BalanceInfo &balance1, const transactions::BalanceInfo &balance2) {
    QCOMPARE(balance1.address, balance2.address);
    QCOMPARE(balance1.countDelegated, balance2.countDelegated);
    QCOMPARE(balance1.countReceived, balance2.countReceived);
    QCOMPARE(balance1.countSpent, balance2.countSpent);
    QCOMPARE(balance1.countTxs, balance2.countTxs);
    QCOMPARE(balance1.currBlockNum, balance2.currBlockNum);
    QCOMPARE(balance1.delegate, balance2.delegate);
    QCOMPARE(balance1.delegated, balance2.delegated);
    QCOMPARE(balance1.forged, balance2.forged);
    QCOMPARE(balance1.received, balance2.received);
    QCOMPARE(balance1.reserved, balance2.reserved);
    QCOMPARE(balance1.spent, balance2.spent);
    QCOMPARE(balance1.undelegate, balance2.undelegate);
    QCOMPARE(balance1.undelegated, balance2.undelegated);
}

void tst_TransactionsDBStorage::tstBalance() {
    if (QFile::exists(transactions::databaseFileName))
        QFile::remove(transactions::databaseFileName);
    transactions::TransactionsDBStorage db;
    db.init();

    transactions::BalanceInfo balance1;
    balance1.address = "addr1";
    balance1.countDelegated = 10;
    balance1.countReceived = 100;
    balance1.countSpent = 101;
    balance1.countTxs = 200;
    balance1.currBlockNum = 1000;
    balance1.delegate = BigNumber(QString("1202239"));

    db.setBalance("cur1", balance1.address, balance1);

    transactions::BalanceInfo balance2;
    balance2.address = "addr1";
    balance2.countDelegated = 11;
    balance2.countReceived = 101;
    balance2.countSpent = 102;
    balance2.countTxs = 200000000;
    balance2.currBlockNum = 1000000000;
    balance2.delegate = BigNumber(QString("1200215463145647002239"));
    balance2.delegated = BigNumber(QString("100"));
    balance2.forged = BigNumber(QString("0"));
    balance2.received = BigNumber(QString("1000"));
    balance2.reserved = BigNumber(QString("233"));
    balance2.spent = BigNumber(QString("2000"));
    balance2.undelegate = BigNumber(QString("343"));
    balance2.undelegated = BigNumber(QString("445"));

    db.setBalance("cur1", balance2.address, balance2);

    transactions::BalanceInfo balance3;
    balance3.address = "addr3";
    balance3.countDelegated = 100;
    balance3.countReceived = 200;
    balance3.countSpent = 201;
    balance3.countTxs = 300000000;
    balance3.currBlockNum = 4000000000;
    balance3.delegate = BigNumber(QString("3145647002239"));
    balance3.delegated = BigNumber(QString("1000100000000000000"));
    balance3.forged = BigNumber(QString("0"));
    balance3.received = BigNumber(QString("10001"));
    balance3.reserved = BigNumber(QString("546368"));
    balance3.spent = BigNumber(QString("3000"));
    balance3.undelegate = BigNumber(QString("5543"));
    balance3.undelegated = BigNumber(QString("41445"));

    db.setBalance("cur1", balance3.address, balance3);

    compareBalances(balance2, db.getBalance("cur1", balance2.address));
    compareBalances(balance3, db.getBalance("cur1", balance3.address));
}

void tst_TransactionsDBStorage::testBigNumSum()
{
    if (QFile::exists(transactions::databaseFileName))
        QFile::remove(transactions::databaseFileName);
    transactions::TransactionsDBStorage db;
    db.init();
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", 1, "user7", "user1", "9000000000000000000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrkgklgfmjgfhg", "address100", 1, "user7", "user1", "9000000000000000000", 568869455887, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrklblgfmjgfhg", "address100", 1, "user7", "user1", "9000000000000000000", 568869455888, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrklklgssjgfhg", "address100", 1, "user7", "user1", "9000000000000000000", 568869455889, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", 2, "user7", "user1", "9000000000000000000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrkgklgfmjgfhg", "address100", 2, "user7", "user1", "9000000000000000000", 568869455887, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrklblgfmjgfhg", "address100", 2, "user7", "user1", "9000000000000000000", 568869455888, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrklklgssjgfhg", "address100", 2, "user7", "user1", "9000000000000000000", 568869455889, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);

    db.addPayment("mh", "gfklklkltrklklgssjgfhg", "address100", false, "user7", "user1", "9000000000000000000", 568869455889, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
}

void tst_TransactionsDBStorage::testGetPayments()
{
    if (QFile::exists(transactions::databaseFileName))
        QFile::remove(transactions::databaseFileName);
    transactions::TransactionsDBStorage db;
    db.init();
    auto transactionGuard = db.beginTransaction();
    for (int n = 0; n < 100; n++) {
        db.addPayment("mh", QString("gfklklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address100", 3, "user7", "user1", "9000000000000000000", 1000 + 2 * n, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "kghkghk", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
        db.addPayment("mh", QString("ggrlklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address20", 3, "user7", "user1", "1000000000000000000", 1000 + 2 * n + 1, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "gffkl", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    }
    transactionGuard.commit();
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
    if (QFile::exists(transactions::databaseFileName))
        QFile::remove(transactions::databaseFileName);
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

void tst_TransactionsDBStorage::testBlockNumer() {
    if (QFile::exists(transactions::databaseFileName))
        QFile::remove(transactions::databaseFileName);
    transactions::TransactionsDBStorage db;
    db.init();

    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", 1, "user7", "user1", "9000000000000000000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11112, "", 1);
    db.addPayment("mh", "gfklklkltrklklgfmjgfhg", "address100", 1, "user7", "user1", "9000000000000000000", 568869455886, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100", "jkgh", transactions::Transaction::OK, transactions::Transaction::SIMPLE, 11113, "", 1);

    long long tt = db.getPaymentsCountForAddress("address100", "mh");
    QCOMPARE(tt, 2);
}

QTEST_MAIN(tst_TransactionsDBStorage)
