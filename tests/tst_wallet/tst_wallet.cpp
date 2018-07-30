#include "tst_wallet.h"

#include <QDebug>

#include <iostream>

#include "btctx/wif.h"
#include "Wallet.h"
#include "EthWallet.h"
#include "BtcWallet.h"
#include "utils.h"
#include "openssl_wrapper/openssl_wrapper.h"

#include "check.h"

Q_DECLARE_METATYPE(std::string)


tst_Wallet::tst_Wallet(QObject *parent)
    : QObject(parent)
{
    InitOpenSSL();
}

///////////
/// MTH ///
///////////

void tst_Wallet::testCreateBinMthTransaction_data() {
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long long>("amount");
    QTest::addColumn<unsigned long long>("fee");
    QTest::addColumn<unsigned long long>("nonce");
    QTest::addColumn<std::string>("answer");

    QTest::newRow("CreateBinTransaction 1")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 126894ULL << 55647ULL << 255ULL
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbaeef0100fa5fd9faff0000");

    QTest::newRow("CreateBinTransaction 2")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 0ULL << 0ULL << 0ULL
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aab00000000");

    QTest::newRow("CreateBinTransaction 3")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 4294967295ULL << 65535ULL << 249ULL
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbfffffffffafffff900");

    QTest::newRow("CreateBinTransaction 4")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
        << 4294967296ULL << 65536ULL << 250ULL
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfc0000000001000000fb00000100fafa0000");
}

void tst_Wallet::testCreateBinMthTransaction() {
    QFETCH(std::string, address);
    QFETCH(unsigned long long, amount);
    QFETCH(unsigned long long, fee);
    QFETCH(unsigned long long, nonce);
    QFETCH(std::string, answer);

    QCOMPARE(toHex(Wallet::genTx(address, amount, fee, nonce, "")), answer);
}

void tst_Wallet::testNotCreateBinMthTransaction_data() {
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long long>("amount");
    QTest::addColumn<unsigned long long>("fee");
    QTest::addColumn<unsigned long long>("nonce");
    QTest::addColumn<std::string>("answer");

    // incorrect address
    QTest::newRow("NotCreateBinTransaction 1")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aa")
        << 126894ULL << 55647ULL << 255ULL
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbaeef0100fa5fd9faff0000");

    // incorrect address
    QTest::newRow("NotCreateBinTransaction 2")
        << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6a")
        << 126894ULL << 55647ULL << 255ULL
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbaeef0100fa5fd9faff0000");

    // incorrect address
    QTest::newRow("NotCreateBinTransaction 3")
        << std::string("0x009806d22a73b1589f38630649bdee48467946d118059efd6aa")
        << 126894ULL << 55647ULL << 255ULL
        << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbaeef0100fa5fd9faff0000");
}

void tst_Wallet::testNotCreateBinMthTransaction() {
    QFETCH(std::string, address);
    QFETCH(unsigned long long, amount);
    QFETCH(unsigned long long, fee);
    QFETCH(unsigned long long, nonce);
    QFETCH(std::string, answer);

    QVERIFY_EXCEPTION_THROWN(Wallet::genTx(address, amount, fee, nonce, ""), TypedException);
}

void tst_Wallet::testCreateMth_data() {
    QTest::addColumn<std::string>("passwd");

    QTest::newRow("CreateMth 1") << std::string("1");
    QTest::newRow("CreateMth 2") << std::string("123");
    QTest::newRow("CreateMth 3") << std::string("Password 1");
    QTest::newRow("CreateMth 4") << std::string("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void tst_Wallet::testCreateMth() {
    QFETCH(std::string, passwd);
    std::string tmp;
    std::string address;
    Wallet::createWallet("./", passwd, tmp, address);
    Wallet wallet("./", address, passwd);
}

void tst_Wallet::testNotCreateMth() {
    std::string tmp;
    std::string address;
    QVERIFY_EXCEPTION_THROWN(Wallet::createWallet("./", "", tmp, address), TypedException);
}

void tst_Wallet::testMthSignTransaction_data() {
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

void tst_Wallet::testMthSignTransaction() {
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

void tst_Wallet::testHashMth_data() {
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

void tst_Wallet::testHashMth() {
    QFETCH(std::string, transaction);
    QFETCH(std::string, answer);

    const std::string result = Wallet::calcHash(transaction);
    QCOMPARE(result, answer);
}

///////////
/// BTC ///
///////////

void tst_Wallet::testEncryptBtc_data() {
    QTest::addColumn<std::string>("wif");
    QTest::addColumn<std::string>("encriptedWif");
    QTest::addColumn<std::string>("npassphraze");
    QTest::addColumn<std::string>("hex");

    QTest::newRow("EncryptBtc 1")
        << std::string("5KN7MzqK5wt2TP1fQCYyHBtDrXdJuXbUzm4A9rKAteGu3Qi5CVR")
        << std::string("6PRVWUbkzzsbcVac2qwfssoUJAN1Xhrg6bNk8J7Nzm5H7kxEbn2Nh2ZoGg")
        << QString("TestingOneTwoThree").normalized(QString::NormalizationForm_C).toStdString()
        << std::string("54657374696e674f6e6554776f5468726565");

    QTest::newRow("EncryptBtc 2")
        << std::string("L44B5gGEpqEDRS9vVPz7QT35jcBG2r3CZwSwQ4fCewXAhAhqGVpP")
        << std::string("6PYNKZ1EAgYgmQfmNVamxyXVWHzK5s6DGhwP4J5o44cvXdoY7sRzhtpUeo")
        << QString("TestingOneTwoThree").normalized(QString::NormalizationForm_C).toStdString()
        << std::string("54657374696e674f6e6554776f5468726565");

#ifndef TARGET_WINDOWS
    QTest::newRow("EncryptBtc 3")
        << std::string("5Jajm8eQ22H3pGWLEVCXyvND8dQZhiQhoLJNKjYXk9roUFTMSZ4")
        << std::string("6PRW5o9FLp4gJDDVqJQKJFTpMvdsSGJxMYHtHaQBF3ooa8mwD69bapcDQn")
        << QString::fromStdString(std::string("\u03D2\u0301\u0000\U00010400\U0001F4A9", 13)).normalized(QString::NormalizationForm_C, QChar::Unicode_2_0).toStdString()
        << std::string("cf9300f0909080f09f92a9");;
#endif
}

void tst_Wallet::testEncryptBtc() {
    QFETCH(std::string, wif);
    QFETCH(std::string, encriptedWif);
    QFETCH(std::string, npassphraze);
    QFETCH(std::string, hex);

    QCOMPARE(toHex(npassphraze), hex);
    QCOMPARE(encryptWif(wif, npassphraze), encriptedWif);
    QCOMPARE(decryptWif(encriptedWif, npassphraze), wif);
}

void tst_Wallet::testCreateBtc_data() {
    QTest::addColumn<QString>("passwd");

    QTest::newRow("CreateBtc 0") << QStringLiteral("");
    QTest::newRow("CreateBtc 1") << QStringLiteral("1");
    QTest::newRow("CreateBtc 2") << QStringLiteral("123");
    QTest::newRow("CreateBtc 3") << QStringLiteral("Password 1");
    QTest::newRow("CreateBtc 4") << QStringLiteral("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void tst_Wallet::testCreateBtc() {
    QFETCH(QString, passwd);
    const std::string address = BtcWallet::genPrivateKey("./", passwd).first;
    BtcWallet wallet("./", address, passwd);
    QCOMPARE(address, wallet.getAddress());
}

void tst_Wallet::testBitcoinTransaction_data() {
    QTest::addColumn<std::string>("wif");
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long long>("amount");
    QTest::addColumn<unsigned long long>("fee");
    QTest::addColumn<QVariantList>("ins");
    QTest::addColumn<std::string>("answer");
    QTest::addColumn<bool>("isTestnet");

    QTest::newRow("BitcoinTransaction 1")
        << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
        << 13240000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("f49da89eba6ef0d4935bf2edf54700710327be0bbdc5db411ad7f016e51ef922")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(13250000ULL)})}
        << std::string("010000000122f91ee516f0d71a41dbc5bd0bbe2703710047f5edf25b93d4f06eba9ea89df4000000006a47304402203292c3b97569f90b2c4458d2b8efdeaa0fbe1f8e65840eec99bcbc626911d5f302200b5ebf256033138129563286d525145a6e7b83fc85f2cc60295290064afd89e9012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff01c006ca00000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac00000000")
        << true;

    QTest::newRow("BitcoinTransaction 2")
        << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
        << 2000000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("90ecb1c712d4d9eb831d13db141726460578718382587b1ab1a3cfeaa8c472d5")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(4000000ULL)})}
        << std::string("0100000001d572c4a8eacfa3b11a7b58828371780546261714db131d83ebd9d412c7b1ec90000000006a473044022067ac4d831d9670d63856e7ce6aa7ecfffca2dc07fb32eef7edaafe35bf9c30c402204472fc396865501cfbe9955f0f5476cd4f9f5a95b7d7658eb781dcc6873042b9012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280841e00000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac705d1e00000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000")
        << true;

    QTest::newRow("BitcoinTransaction 3")
        << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
        << 6920000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("6c11ec775245041f7679d4bace0525312dc9583dede012109e8b100e6b867dce")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1220000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("9f032eb20006520da112364bd91c8e0e75809aab3160fb99a78df40e783064e0")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(4470000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("0afece19c37770b3be801cf8305666fc1cb2a5fd3d8989cf46a3a4298a39c3c3")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1240000ULL)})}
        << std::string("0100000003ce7d866b0e108b9e1012e0ed3d58c92d312505cebad479761f04455277ec116c000000006b483045022100b3d988166d3b5b7c8507839f5d4e902dec8f772ccd78706caf587f5827a2863a02207ceb4611a104e455b3321a8fa4fa9829cc6241ebeacacb7c5c07d7bb56b295e1012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffe06430780ef48da799fb6031ab9a80750e8e1cd94b3612a10d520600b22e039f000000006a473044022050049602594bb1822b3c6dc1a3f0ed0c1247f108758d250e8b95dd8347b0a2cc0220707862301f0f0f7ed76a71545a6b604f95490046d6e31ba531cac3811c920090012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffc3c3398a29a4a346cf89893dfda5b21cfc665630f81c80beb37077c319cefe0a000000006b483045022100da15925c0ffc2fe2c385166399977c7bc0da8adcf78f23b1cacaa7de3e030742022007f720af79399ee9ec66ab164b7d5f026d891a175586977c3097b734dcacc35e012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0140976900000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac00000000")
        << true;

    QTest::newRow("BitcoinTransaction 4")
        << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
        << 50000000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("72ecdaf25a178f6879c4d879551a06b2f0344ca137de3e5afb7820cdb57722b8")), QVariant(1U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1990000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("94026dae0058bd0059b84e9910e5f0f30153f4b78cdeb8f1b59b54ad72bd98ca")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(100000000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("0c48634b6ebf0a07430b1c08b53df81159d24902346d396bfd2d3cba2852e384")), QVariant(1U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1415000ULL)})}
        << std::string("0100000003b82277b5cd2078fb5a3ede37a14c34f0b2061a5579d8c479688f175af2daec72010000006b483045022100e331f87c1da0dc1f25bcd28ffd61e96339666fc5ca4729e89b2e940061fcdd6d022011748b41655046934c9e124a4ae307eb8f67a8c61b9d957e2cca0b95fcb21397012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffca98bd72ad549bb5f1b8de8cb7f45301f3f0e510994eb85900bd5800ae6d0294000000006b483045022100938db1a910e54efdf55cc5228bb7c77b5e40b19ef30eaa60ca45d40fab7f316602202bf5298ad3cfc0cb461491b31303138ad74ff61537193cb4d9b24d50a4a3a36e012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff84e35228ba3c2dfd6b396d340249d25911f83db5081c0b43070abf6e4b63480c010000006b483045022100ff7bc69634e30b614068733750e7c6e8a6906f3116f26bf9d010289bc5b185fb0220467427fd7fce3fb97f217716c00f764354ad929fb527d3b57448667b222080fc012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280f0fa02000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac38be2e03000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000")
        << true;

    QTest::newRow("BitcoinTransaction 5")
        << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("1Lk1p9yr2StBnGFtMeqnLHpf8oGL3WdeBM")
        << 50000000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("72ecdaf25a178f6879c4d879551a06b2f0344ca137de3e5afb7820cdb57722b8")), QVariant(1U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1990000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("94026dae0058bd0059b84e9910e5f0f30153f4b78cdeb8f1b59b54ad72bd98ca")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(100000000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("0c48634b6ebf0a07430b1c08b53df81159d24902346d396bfd2d3cba2852e384")), QVariant(1U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1415000ULL)})}
        << std::string("0100000003b82277b5cd2078fb5a3ede37a14c34f0b2061a5579d8c479688f175af2daec72010000006a473044022046b07a0e26731856b3d0ec54cc424a840cba0f09f8a0c40ae0049b37e49ccd95022002b181b3e014721792d1c0e7b9cb840b6903cd647d03d041d524f6e464390f17012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffca98bd72ad549bb5f1b8de8cb7f45301f3f0e510994eb85900bd5800ae6d0294000000006b483045022100baf04b91ac274f0c4f11dcf5bc12fae4b2500966fac4ef2692d53bbc085a0aa9022032e98d730e2d48a9405a8aeff95c7c5ad0a0ba854a2ad0ce6ac005695627de2d012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff84e35228ba3c2dfd6b396d340249d25911f83db5081c0b43070abf6e4b63480c010000006b48304502210088a617310496cd03b0b21b3b13cd9a2e3e4f9956e1480978de326526af3b7aea022004991c3ee5c4ff4059e20c1ec7b7fec86e67ecbaa328970884d2ff1f24c802d8012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280f0fa02000000001976a914d88cf4759024083e3837eb78b7fb75641c41c75c88ac38be2e03000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000")
        << false;

    QTest::newRow("BitcoinTransaction 6")
        << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("3FnoJdQLXto5GFUcf4xkBGnGZ1JvQQ6rDD")
        << 50000000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("72ecdaf25a178f6879c4d879551a06b2f0344ca137de3e5afb7820cdb57722b8")), QVariant(1U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1990000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("94026dae0058bd0059b84e9910e5f0f30153f4b78cdeb8f1b59b54ad72bd98ca")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(100000000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("0c48634b6ebf0a07430b1c08b53df81159d24902346d396bfd2d3cba2852e384")), QVariant(1U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1415000ULL)})}
        << std::string("0100000003b82277b5cd2078fb5a3ede37a14c34f0b2061a5579d8c479688f175af2daec72010000006b483045022100af162e02d29287f521024b2bcb8a40f9fe37c12ffd56ce2d67dc977c44f06b9a022029c551383f1a306f56546774629b2f51bbab520984a0fa5899c3aa508212b7a4012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffca98bd72ad549bb5f1b8de8cb7f45301f3f0e510994eb85900bd5800ae6d0294000000006b4830450221009e3724bc268dc494a83c61497be110276cf322f7845a5252bcab1f12c15a415d0220134a84ed36b650d8831c7fdee016618a6a69dd0fea92f1ca1ff7c5ffd914a888012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff84e35228ba3c2dfd6b396d340249d25911f83db5081c0b43070abf6e4b63480c010000006a4730440220074d9ea95853e126309d94d5839d3d14478f9eb1306d160858646b27f26a4dab02202502836c9f45fc830f343c5bc487c80d4a74a06bbc137f163ccb1958d3b5ec41012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280f0fa020000000017a9149aa9b905937c86d543bbd666b3374cb8aa74766e8738be2e03000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000")
        << false;
}

void tst_Wallet::testBitcoinTransaction() {
    QFETCH(std::string, wif);
    QFETCH(std::string, address);
    QFETCH(unsigned long long, amount);
    QFETCH(unsigned long long, fee);
    QFETCH(QVariantList, ins);
    QFETCH(std::string, answer);
    QFETCH(bool, isTestnet);

    std::vector<BtcInput> is;
    BtcInput input;
    foreach (const QVariant &vin, ins) {
        QVariantList in = vin.toList();
        input.spendtxid = in.at(0).value<std::string>();
        input.spendoutnum = in.at(1).toUInt();
        input.scriptPubkey = in.at(2).value<std::string>();
        input.outBalance = in.at(3).toULongLong();
        is.push_back(input);
    }

    BtcWallet wallet(wif);
    const std::string tx = wallet.genTransaction(is, amount, fee, address, isTestnet);
    QCOMPARE(tx, answer);
}

void tst_Wallet::testNotCreateBtcTransaction_data() {
    QTest::addColumn<std::string>("wif");
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long long>("amount");
    QTest::addColumn<unsigned long long>("fee");
    QTest::addColumn<QVariantList>("ins");
    QTest::addColumn<std::string>("answer");

    // Неправильный wif
    QTest::newRow("NotBitcoinTransaction 1")
        << std::string("cUzkK2uj56xSuwY2Ha9TMlKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
        << 13240000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("f49da89eba6ef0d4935bf2edf54700710327be0bbdc5db411ad7f016e51ef922")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(13250000ULL)})}
        << std::string("010000000122f91ee516f0d71a41dbc5bd0bbe2703710047f5edf25b93d4f06eba9ea89df4000000006a47304402203292c3b97569f90b2c4458d2b8efdeaa0fbe1f8e65840eec99bcbc626911d5f302200b5ebf256033138129563286d525145a6e7b83fc85f2cc60295290064afd89e9012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff01c006ca00000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac00000000");

    // Неправильный address
    QTest::newRow("NotBitcoinTransaction 4")
        << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("mkDQ29a4WtweYxagkhwuTx8P6BtsTnkJwi")
        << 50000000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("72ecdaf25a178f6879c4d879551a06b2f0344ca137de3e5afb7820cdb57722b8")), QVariant(1U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1990000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("94026dae0058bd0059b84e9910e5f0f30153f4b78cdeb8f1b59b54ad72bd98ca")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(100000000ULL)}),
                QVariant(QVariantList{QVariant::fromValue(std::string("0c48634b6ebf0a07430b1c08b53df81159d24902346d396bfd2d3cba2852e384")), QVariant(1U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1415000ULL)})}
        << std::string("0100000003b82277b5cd2078fb5a3ede37a14c34f0b2061a5579d8c479688f175af2daec72010000006b483045022100e331f87c1da0dc1f25bcd28ffd61e96339666fc5ca4729e89b2e940061fcdd6d022011748b41655046934c9e124a4ae307eb8f67a8c61b9d957e2cca0b95fcb21397012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffca98bd72ad549bb5f1b8de8cb7f45301f3f0e510994eb85900bd5800ae6d0294000000006b483045022100938db1a910e54efdf55cc5228bb7c77b5e40b19ef30eaa60ca45d40fab7f316602202bf5298ad3cfc0cb461491b31303138ad74ff61537193cb4d9b24d50a4a3a36e012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff84e35228ba3c2dfd6b396d340249d25911f83db5081c0b43070abf6e4b63480c010000006b483045022100ff7bc69634e30b614068733750e7c6e8a6906f3116f26bf9d010289bc5b185fb0220467427fd7fce3fb97f217716c00f764354ad929fb527d3b57448667b222080fc012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280f0fa02000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac38be2e03000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000");
}

void tst_Wallet::testNotCreateBtcTransaction() {
    QFETCH(std::string, wif);
    QFETCH(std::string, address);
    QFETCH(unsigned long long, amount);
    QFETCH(unsigned long long, fee);
    QFETCH(QVariantList, ins);
    QFETCH(std::string, answer);

    std::vector<BtcInput> is;
    BtcInput input;
    foreach (const QVariant &vin, ins) {
        QVariantList in = vin.toList();
        input.spendtxid = in.at(0).value<std::string>();
        input.spendoutnum = in.at(1).toUInt();
        input.scriptPubkey = in.at(2).value<std::string>();
        input.outBalance = in.at(3).toULongLong();
        is.push_back(input);
    }

    BtcWallet wallet(wif);
    QVERIFY_EXCEPTION_THROWN(wallet.genTransaction(is, amount, fee, address, true), TypedException);
}

void tst_Wallet::testNotCreateBtcTransaction2_data() {
    QTest::addColumn<std::string>("wif");
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long long>("amount");
    QTest::addColumn<unsigned long long>("fee");
    QTest::addColumn<QVariantList>("ins");
    QTest::addColumn<std::string>("answer");

    // Мало денег
    QTest::newRow("NotBitcoinTransaction 2")
        << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
        << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
        << 2000000ULL << 10000ULL
        << QVariantList{
                QVariant(QVariantList{QVariant::fromValue(std::string("90ecb1c712d4d9eb831d13db141726460578718382587b1ab1a3cfeaa8c472d5")), QVariant(0U),
                    QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(400000ULL)})}
        << std::string("0100000001d572c4a8eacfa3b11a7b58828371780546261714db131d83ebd9d412c7b1ec90000000006a473044022067ac4d831d9670d63856e7ce6aa7ecfffca2dc07fb32eef7edaafe35bf9c30c402204472fc396865501cfbe9955f0f5476cd4f9f5a95b7d7658eb781dcc6873042b9012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280841e00000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac705d1e00000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000");
}

void tst_Wallet::testNotCreateBtcTransaction2() {
    QFETCH(std::string, wif);
    QFETCH(std::string, address);
    QFETCH(unsigned long long, amount);
    QFETCH(unsigned long long, fee);
    QFETCH(QVariantList, ins);
    QFETCH(std::string, answer);

    std::vector<BtcInput> is;
    BtcInput input;
    foreach (const QVariant &vin, ins) {
        QVariantList in = vin.toList();
        input.spendtxid = in.at(0).value<std::string>();
        input.spendoutnum = in.at(1).toUInt();
        input.scriptPubkey = in.at(2).value<std::string>();
        input.outBalance = in.at(3).toULongLong();
        is.push_back(input);
    }

    BtcWallet wallet(wif);
    QVERIFY_EXCEPTION_THROWN(wallet.genTransaction(is, amount, fee, address, true), Exception);
}

void tst_Wallet::testHashBtc_data() {
    QTest::addColumn<std::string>("transaction");
    QTest::addColumn<std::string>("answer");

    QTest::newRow("hashBtc 1")
        << std::string("0100000002d8c8df6a6fdd2addaf589a83d860f18b44872d13ee6ec3526b2b470d42a96d4d000000008b483045022100b31557e47191936cb14e013fb421b1860b5e4fd5d2bc5ec1938f4ffb1651dc8902202661c2920771fd29dd91cd4100cefb971269836da4914d970d333861819265ba014104c54f8ea9507f31a05ae325616e3024bd9878cb0a5dff780444002d731577be4e2e69c663ff2da922902a4454841aa1754c1b6292ad7d317150308d8cce0ad7abffffffff2ab3fa4f68a512266134085d3260b94d3b6cfd351450cff021c045a69ba120b2000000008b4830450220230110bc99ef311f1f8bda9d0d968bfe5dfa4af171adbef9ef71678d658823bf022100f956d4fcfa0995a578d84e7e913f9bb1cf5b5be1440bcede07bce9cd5b38115d014104c6ec27cffce0823c3fecb162dbd576c88dd7cda0b7b32b0961188a392b488c94ca174d833ee6a9b71c0996620ae71e799fc7c77901db147fa7d97732e49c8226ffffffff02c0175302000000001976a914a3d89c53bb956f08917b44d113c6b2bcbe0c29b788acc01c3d09000000001976a91408338e1d5e26db3fce21b011795b1c3c8a5a5d0788ac00000000")
        << std::string("9021b49d445c719106c95d561b9c3fac7bcb3650db67684a9226cd7fa1e1c1a0");

    QTest::newRow("hashBtc 2")
        << std::string("01000000010000000000000000000000000000000000000000000000000000000000000000ffffffff4d04ffff001d0104455468652054696d65732030332f4a616e2f32303039204368616e63656c6c6f72206f6e206272696e6b206f66207365636f6e64206261696c6f757420666f722062616e6b73ffffffff0100f2052a01000000434104678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5fac00000000")
        << std::string("4a5e1e4baab89f3a32518a88c31bc87f618f76673e2cc77ab2127b7afdeda33b");

    QTest::newRow("hashBtc 3")
        << std::string("0100000001601b3ab968208fe3e94b90ca23717bf7de0df6614f3c29aa147bb42e2025c74f000000008a473044022024f1f27ee985fe435028ada38f9f4912751ffabf6dc83f1c12547f23395e222802207223240636ab54b22bdb5f6fbdb4d25becb9118f3b031fa14be3705829c7832f014104c4aef9af61e88cddb20e27a35dd8233f4373fb437ab5582d639d43e27499cbcecc3355fbfed321a9fd0cf15d0f0bd130eb8932c0f33c208e09e265a36a1907ecffffffff01db890000000000001976a9148861c20450e2c4060912896af8c3022cd6afea4e88ac00000000")
        << std::string("77b2dcbf0fdf4729fafe57583c5842e8f9e58d1eb48bd5acac1e36ea37ce8880");
}

void tst_Wallet::testHashBtc() {
    QFETCH(std::string, transaction);
    QFETCH(std::string, answer);

    const std::string result = BtcWallet::calcHashNotWitness(transaction);
    QCOMPARE(result, answer);
}

///////////
/// ETH ///
///////////

void tst_Wallet::testCreateEth_data() {
    QTest::addColumn<std::string>("passwd");

    QTest::newRow("CreateEth 1") << std::string("1");
    QTest::newRow("CreateEth 2") << std::string("123");
    QTest::newRow("CreateEth 3") << std::string("Password 1");
    QTest::newRow("CreateEth 4") << std::string("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void tst_Wallet::testCreateEth() {
    QFETCH(std::string, passwd);
    const std::string address = EthWallet::genPrivateKey("./", passwd);
    EthWallet wallet("./", address, passwd);
}

void tst_Wallet::testEthWalletTransaction()
{
    writeToFile("./123", "{\"address\": \"05cf594f12bba9430e34060498860abc69554cb1\",\"crypto\": {\"cipher\": \"aes-128-ctr\",\"ciphertext\": \"694283a4a2f3da99186e2321c24cf1b427d81a273e7bc5c5a54ab624c8930fb8\",\"cipherparams\": {\"iv\": \"5913da2f0f6cd00b9b62ff2bc0a8b9d3\"},\"kdf\": \"scrypt\",\"kdfparams\": {\"dklen\": 32,\"n\": 262144,\"p\": 1,\"r\": 8,\"salt\": \"ca45d433267bd6a50ace149d6b317b9d8f8a39f43621bad2a3108981bf533ee7\"},\"mac\": \"0a8d581e8c60553970301603ea35b0fc56cbccd5913b12f62c690acb98d111c8\"},\"id\": \"6406896a-2ec9-4dd7-b98e-5fbfc0984e6f\",\"version\": 3}", false);
    const std::string password = "1";
    EthWallet wallet("./", "123", password);
    const std::string result = wallet.SignTransaction(
        "0x01",
        "0x6C088E200",
        "0x8208",
        "0x8D78B1Ab426dc9daa7427b7A60E64633f62E645F",
        "0x746A528800",
        "0x010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101"
    );
    QCOMPARE(result, std::string("0xf899018506c088e200828208948d78b1ab426dc9daa7427b7a60e64633f62e645f85746a528800b001010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010126a047dd9f6ebce749230df9ac9d57db85f948db0775882cb63565501fe95ddfcb58a07c7020426395bc781fc06e4fbb5cffc5c4d8b77d37596b1c83fa0c21ce37cfb3"));
}

void tst_Wallet::testNotCreateEthTransaction_data() {
    QTest::addColumn<std::string>("to");
    QTest::addColumn<std::string>("nonce");
    QTest::addColumn<std::string>("gasPrice");
    QTest::addColumn<std::string>("gasLimit");
    QTest::addColumn<std::string>("value");
    QTest::addColumn<std::string>("data");

    // Incorrect address
    QTest::newRow("NotCreateEthTransaction 1")
        << std::string("0x8D78B1Ab426dc9daa7427b7A60E64633f62E645")
        << std::string("0x01") << std::string("0x6C088E200") << std::string("0x8208") << std::string("0x746A528800")
        << std::string("0x010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101");

    // Incorrect value
    QTest::newRow("NotCreateEthTransaction 2")
        << std::string("0x8D78B1Ab426dc9daa7427b7A60E64633f62E645F")
        << std::string("0x01") << std::string("6C088E200") << std::string("0x8208") << std::string("0x746A528800")
        << std::string("0x010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101");

    // Incorrect data
    QTest::newRow("NotCreateEthTransaction 3")
        << std::string("0x8D78B1Ab426dc9daa7427b7A60E64633f62E645F")
        << std::string("0x01") << std::string("0x6C088E200") << std::string("0x8208") << std::string("0x746A528800")
        << std::string("x010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101");
}

void tst_Wallet::testNotCreateEthTransaction() {
    QFETCH(std::string, to);
    QFETCH(std::string, nonce);
    QFETCH(std::string, gasPrice);
    QFETCH(std::string, gasLimit);
    QFETCH(std::string, value);
    QFETCH(std::string, data);

    writeToFile("./123", "{\"address\": \"05cf594f12bba9430e34060498860abc69554cb1\",\"crypto\": {\"cipher\": \"aes-128-ctr\",\"ciphertext\": \"694283a4a2f3da99186e2321c24cf1b427d81a273e7bc5c5a54ab624c8930fb8\",\"cipherparams\": {\"iv\": \"5913da2f0f6cd00b9b62ff2bc0a8b9d3\"},\"kdf\": \"scrypt\",\"kdfparams\": {\"dklen\": 32,\"n\": 262144,\"p\": 1,\"r\": 8,\"salt\": \"ca45d433267bd6a50ace149d6b317b9d8f8a39f43621bad2a3108981bf533ee7\"},\"mac\": \"0a8d581e8c60553970301603ea35b0fc56cbccd5913b12f62c690acb98d111c8\"},\"id\": \"6406896a-2ec9-4dd7-b98e-5fbfc0984e6f\",\"version\": 3}", false);
    const std::string password = "1";
    EthWallet wallet("./", "123", password);
    QVERIFY_EXCEPTION_THROWN(wallet.SignTransaction(
        nonce,
        gasPrice,
        gasLimit,
        to,
        value,
        data
    ), TypedException);
}

void tst_Wallet::testHashEth_data() {
    QTest::addColumn<std::string>("transaction");
    QTest::addColumn<std::string>("answer");

    QTest::newRow("hashEth 1")
        << std::string("0xf8a802843b9aca0082916a9474fd51a98a4a1ecbef8cc43be801cce630e260bd80b844a9059cbb000000000000000000000000a5c9f8d2fd5ced0101391d1eb444b75145ed35a700000000000000000000000000000000000000000000048d0aeb379680eb2e9425a04efaf4e730b5bdcfb2ee691028887ed86dfc6524adb444e053be20da7c9238f6a00d1468a7bb406e8c0c551e2f586555b19a20a4b85b11d1e9056db7c9dc7dc65d")
        << std::string("0x389d9ae5dbb5a80c0f2c5dc5d424dd83d42f5f6a79f058650e6be40d721a7adc");

    QTest::newRow("hashEth 2")
        << std::string("0xf86415843b9aca00830130fe94e1144e3cfda7a6d40ae60a968c1edc8ad54aca70808025a030f7f9793949a7194458457bb5afd03ccdc1b09408e667efbcb37625007691e0a0064255fbf3bfde2344f24cad9c63f67ce9a93e60009e6e8aa64017d3aa72deec")
        << std::string("0x6162c8ca2d0e34c32604a3c23bbfbf177bb31d7a937fae312708eadbb196f5a1");

    QTest::newRow("hashEth 3")
        << std::string("0xf8a909843b9aca00830186a0940cfda67b0067f1a99deb1cb80e0273a3f26d317c80b844a9059cbb000000000000000000000000001bb9990e8b17f83eb575a6d8958cc7c5c1c51f0000000000000000000000000000000000000000000000e4b66650b9c8aee3681ca05bdcddecbd440184d10c0537ea8035b5a6d3cde873789b6b96dc7183a35b31dca01ebdb5d446a81cff7b2e45a2bc4a572c51a535133164115356e9f9c9b5d4b31f")
        << std::string("0xb47177f79a8a7f16e9444ee56a37856aeaaf4c132017699328074f5a40b9c76c");

    QTest::newRow("hashEth 4")
        << std::string("0xf8a806843b9aca0082ea60944f38f4229924bfa28d58eeda496cc85e8016bccc80b844a9059cbb00000000000000000000000028ae5cf24c91813d0887dbed5eb0d6ec0b1f4bae000000000000000000000000000000000000000000000000000000000000001426a0709af81dd4cc71089839dd29143a2fee4eb4380c6c01b9cdd1a1273d992d43dfa06acb67efc7b05c0e7419bea272e8061a1ad30db0d72b315736f9b560d8145085")
        << std::string("0x656eb3735d912f4e1b284781a04d712e13b60587757aeb6edd0203dd5d7285c7");

    QTest::newRow("hashEth 5")
        << std::string("0xf8ab820104843b9aca008309eb1094511bc4556d823ae99630ae8de28b9b80df90ea2e80b844b78d27dc00000000000000000000000000000000000000000000000055b9313b48f75f58000000000000000000000000be9a8b21353ea73794553c154c6c9a06823fcd1126a0f467d956673d6646d1735621e631015d9474028a99097622938e388dacaef3f7a0051b8b7e8ef574c435c4b6c421530842a26db2770f3525784fbc87ccc4c7995f")
        << std::string("0x281bb537ef5ee9143cdf3051b8d8f2fd16a57a46b2600393d0916cebbe0c5b2e");
}

void tst_Wallet::testHashEth() {
    QFETCH(std::string, transaction);
    QFETCH(std::string, answer);

    const std::string result = EthWallet::calcHash(transaction);
    QCOMPARE(result, answer);
}

///////////
/// RSA ///
///////////

void tst_Wallet::testSsl_data() {
    QTest::addColumn<std::string>("password");
    QTest::addColumn<std::string>("message");

    QTest::newRow("Ssl 1")
        << std::string("")
        << std::string("Message 1");
    QTest::newRow("Ssl 2")
        << std::string("1")
        << std::string("Message 2");
    QTest::newRow("Ssl 3")
        << std::string("123")
        << std::string("Message 3");
    QTest::newRow("Ssl 4")
        << std::string("Password 1")
        << std::string("Message 4");
    QTest::newRow("Ssl 5")
        << std::string("Password 1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111")
        << std::string("Message 4");

    std::string rr;
    for (size_t i = 0; i < 10000; i++) {
        rr += (char)(i % 256);
    }

    QTest::newRow("Ssl 6")
        << std::string("Password 1")
        << rr;
}

#include <iostream>

void tst_Wallet::testSsl() {
    QFETCH(std::string, password);
    QFETCH(std::string, message);

    const std::string privateKey = createRsaKey(password);
    const std::string publicKey = getPublic(privateKey, password);

    const std::string encryptedMsg = encrypt(publicKey, message);
    const std::string decryptMsg = decrypt(privateKey, password, encryptedMsg);

    QCOMPARE(decryptMsg, message);
}

QTEST_MAIN(tst_Wallet)
