#include "tst_Metahash.h"

#include "Wallet.h"

#include "utils.h"
#include "check.h"

#include "openssl_wrapper/openssl_wrapper.h"

Q_DECLARE_METATYPE(std::string)

tst_Metahash::tst_Metahash(QObject *parent) : QObject(parent) {
    if (!isInitOpenSSL()) {
        InitOpenSSL();
    }
}

void tst_Metahash::testCreateBinMthTransaction_data() {
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long long>("amount");
    QTest::addColumn<unsigned long long>("fee");
    QTest::addColumn<unsigned long long>("nonce");
    QTest::addColumn<std::string>("data");
    QTest::addColumn<std::string>("answer");

    QTest::newRow("CreateBinTransaction 1")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 126894ULL << 55647ULL << 255ULL
        << std::string()
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbaeef0100fa5fd9faff0000");

    QTest::newRow("CreateBinTransaction 2")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 0ULL << 0ULL << 0ULL
        << std::string()
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aab00000000");

    QTest::newRow("CreateBinTransaction 3")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 4294967295ULL << 65535ULL << 249ULL
        << std::string()
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbfffffffffafffff900");

    QTest::newRow("CreateBinTransaction 4")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 4294967296ULL << 65536ULL << 250ULL
        << std::string()
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfc0000000001000000fb00000100fafa0000");

    QTest::newRow("CreateBinTransaction 5")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 126894ULL << 55647ULL << 255ULL
        << std::string("4d79207465787420225c2027")
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbaeef0100fa5fd9faff000c4d79207465787420225c2027");

}

void tst_Metahash::testCreateBinMthTransaction() {
    QFETCH(std::string, address);
    QFETCH(unsigned long long, amount);
    QFETCH(unsigned long long, fee);
    QFETCH(unsigned long long, nonce);
    QFETCH(std::string, data);
    QFETCH(std::string, answer);

    QCOMPARE(toHex(Wallet::genTx(address, amount, fee, nonce, data)), answer);
}

void tst_Metahash::testNotCreateBinMthTransaction_data() {
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long long>("amount");
    QTest::addColumn<unsigned long long>("fee");
    QTest::addColumn<unsigned long long>("nonce");

    // incorrect address
    QTest::newRow("NotCreateBinTransaction 1")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aa")
        << 126894ULL << 55647ULL << 255ULL;

    // incorrect address
    QTest::newRow("NotCreateBinTransaction 2")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6a")
        << 126894ULL << 55647ULL << 255ULL;

    // incorrect address
    QTest::newRow("NotCreateBinTransaction 3")
        << std::string("0x009806d22a73b1589f38630649bdee48467946d118059efd6aa")
        << 126894ULL << 55647ULL << 255ULL;
}

void tst_Metahash::testNotCreateBinMthTransaction() {
    QFETCH(std::string, address);
    QFETCH(unsigned long long, amount);
    QFETCH(unsigned long long, fee);
    QFETCH(unsigned long long, nonce);

    QVERIFY_EXCEPTION_THROWN(Wallet::genTx(address, amount, fee, nonce, ""), TypedException);
}

void tst_Metahash::testCreateMth_data() {
    QTest::addColumn<std::string>("passwd");

    QTest::newRow("CreateMth 1") << std::string("1");
    QTest::newRow("CreateMth 2") << std::string("123");
    QTest::newRow("CreateMth 3") << std::string("Password 1");
    QTest::newRow("CreateMth 4") << std::string("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void tst_Metahash::testCreateMth() {
    QFETCH(std::string, passwd);
    std::string tmp;
    std::string address;
    Wallet::createWallet("./", passwd, tmp, address);
    Wallet wallet("./", address, passwd);
}

void tst_Metahash::testNotCreateMth() {
    std::string tmp;
    std::string address;
    QVERIFY_EXCEPTION_THROWN(Wallet::createWallet("./", "", tmp, address), TypedException);
}

void tst_Metahash::testMthSignTransaction_data() {
    QTest::addColumn<std::string>("message");

    QTest::newRow("MthSign 1") << std::string("1");
    QTest::newRow("MthSign 2") << std::string("1565144654fdsfadsafs");
    QTest::newRow("MthSign 3") << std::string("1dsfadsfdasfdsafdsafe3234543tdfsdt435234adsfear34554tgdfasdf435234tgfdsafadsf4t54tdfsadsf4tdfsdafjhlkjhdsf745739485hlhjhl");

    std::string r;
    for (unsigned char c = 1; c < 255; c++) {
        r += c;
    }
    r += '\0';
    r += "data";
    QTest::newRow("MthSign 4") << r;
}

void tst_Metahash::testMthSignTransaction() {
    QFETCH(std::string, message);
    std::string tmp;
    std::string address;
    Wallet::createWallet("./", "123", tmp, address);
    Wallet wallet("./", address, "123");
    std::string pubkey;
    const std::string result = wallet.sign(message, pubkey);
    const bool res = Wallet::verify(message, result, pubkey);
    QCOMPARE(res, true);

    const bool res2 = Wallet::verify(message.substr(0, message.size() / 2), result, pubkey);
    QCOMPARE(res2, false);
}

void tst_Metahash::testHashMth_data() {
    QTest::addColumn<std::string>("transaction");
    QTest::addColumn<std::string>("answer");

    QTest::newRow("hashMth 1")
        << std::string("00e1c97266d97bf475ee6128f1ad1e8de33cfcc8da0927fc4bfb00286bee0000000000")
        << std::string("c9b70ccdb83fce54a80038b32513b23b08152573c58efc48a24aea64a4a2e758");

    QTest::newRow("hashMth 2")
        << std::string("0052fa7e6e55c8cd2b7ed9552b57074e71c791352a601f7d1cfb00286bee0000000000")
        << std::string("091ae3f829c28c0c158f5764592393a79d9f2cf62a9ced7250537fec32d04de9");

    QTest::newRow("hashMth 3")
        << std::string("008b09aaaa52cf75ef2cfb9dbfff27456d603580fa2b4b79d0fb00286bee0000000000")
        << std::string("676c93644afd5715bf7d7d136b1157d66c346709bd3ba87a2f0b3792694502ca");

    QTest::newRow("hashMth 4")
        << std::string("00c44b358872bd6eba5d82f61769276e8954eb3c46bb3ba451fb00286bee0000000000")
        << std::string("37193cede0bacfd28567fd4b8a87cff9474ffc846cd5fdba499ed8900b2876e1");

    QTest::newRow("hashMth 5")
        << std::string("0066433cf1d4ba40e8aac98d874447d6c1a4982ea56fc26a61fb00286bee0000000000")
        << std::string("a4d0246668398310111e89b401cee3b028cf3dba3045f76d6d4488271685d47f");
}

void tst_Metahash::testHashMth() {
    QFETCH(std::string, transaction);
    QFETCH(std::string, answer);

    const std::string result = Wallet::calcHash(transaction);
    QCOMPARE(result, answer);
}
