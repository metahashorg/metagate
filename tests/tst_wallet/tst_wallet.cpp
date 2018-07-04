#include "tst_wallet.h"

#include <QDebug>

#include "btctx/wif.h"
#include "Wallet.h"
#include "EthWallet.h"
#include "BtcWallet.h"
#include "utils.h"
#include "openssl_wrapper/openssl_wrapper.h"

Q_DECLARE_METATYPE(std::string)


tst_Wallet::tst_Wallet(QObject *parent)
    : QObject(parent)
{
    InitOpenSSL();
}

void tst_Wallet::testEncryptBtc_data()
{
    QTest::addColumn<std::string>("wif");
    QTest::addColumn<std::string>("encriptedWif");
    QTest::addColumn<std::string>("npassphraze");

    QTest::newRow("EncryptBtc 1") << std::string("5KN7MzqK5wt2TP1fQCYyHBtDrXdJuXbUzm4A9rKAteGu3Qi5CVR")
                                  << std::string("6PRVWUbkzzsbcVac2qwfssoUJAN1Xhrg6bNk8J7Nzm5H7kxEbn2Nh2ZoGg")
                                  << QString("TestingOneTwoThree").normalized(QString::NormalizationForm_C).toStdString();

    QTest::newRow("EncryptBtc 2") << std::string("L44B5gGEpqEDRS9vVPz7QT35jcBG2r3CZwSwQ4fCewXAhAhqGVpP")
                                  << std::string("6PYNKZ1EAgYgmQfmNVamxyXVWHzK5s6DGhwP4J5o44cvXdoY7sRzhtpUeo")
                                  << QString("TestingOneTwoThree").normalized(QString::NormalizationForm_C).toStdString();
#ifndef TARGET_WINDOWS
    QTest::newRow("EncryptBtc 3") << std::string("5Jajm8eQ22H3pGWLEVCXyvND8dQZhiQhoLJNKjYXk9roUFTMSZ4")
                                  << std::string("6PRW5o9FLp4gJDDVqJQKJFTpMvdsSGJxMYHtHaQBF3ooa8mwD69bapcDQn")
                                  << QString::fromStdString(std::string("\u03D2\u0301\u0000\U00010400\U0001F4A9", 13)).
                                     normalized(QString::NormalizationForm_C, QChar::Unicode_2_0).toStdString();
#endif
}

void tst_Wallet::testEncryptBtc()
{
    QFETCH(std::string, wif);
    QFETCH(std::string, encriptedWif);
    QFETCH(std::string, npassphraze);
    QCOMPARE(encryptWif(wif, npassphraze), encriptedWif);
    QCOMPARE(decryptWif(encriptedWif, npassphraze), wif);
}

void tst_Wallet::testCreateBinTransaction_data()
{
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long>("amount");
    QTest::addColumn<unsigned long>("fee");
    QTest::addColumn<unsigned long>("nonce");
    QTest::addColumn<std::string>("answer");

    QTest::newRow("CreateBinTransaction 1") << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
                                            << 126894UL << 55647UL << 255UL
                                            << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbaeef0100fa5fd9faff0000");
    QTest::newRow("CreateBinTransaction 2") << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
                                            << 0UL << 0UL << 0UL
                                            << std::string("009806da73b1589f38630649bdee48467946d118059efd6aab00000000");
    QTest::newRow("CreateBinTransaction 3") << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
                                            << 4294967295UL << 65535UL << 249UL
                                            << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfbfffffffffafffff900");
    QTest::newRow("CreateBinTransaction 4") << std::string("0x009806da73b1589f38630649bdee48467946d118059efd6aab")
                                            << 4294967296UL << 65536UL << 250UL
                                            << std::string("009806da73b1589f38630649bdee48467946d118059efd6aabfc0000000001000000fb00000100fafa0000");
}

void tst_Wallet::testCreateBinTransaction()
{
    QFETCH(std::string, address);
    QFETCH(unsigned long, amount);
    QFETCH(unsigned long, fee);
    QFETCH(unsigned long, nonce);
    QFETCH(std::string, answer);

    QCOMPARE(toHex(Wallet::genTx(address, amount, fee, nonce, "")), answer);
}

void tst_Wallet::testSsl_data()
{
    QTest::addColumn<std::string>("password");
    QTest::addColumn<std::string>("message");

    QTest::newRow("Ssl 1") << std::string("")
                           << std::string("Message 1");
    QTest::newRow("Ssl 2") << std::string("1")
                           << std::string("Message 2");
    QTest::newRow("Ssl 3") << std::string("123")
                           << std::string("Message 3");
    QTest::newRow("Ssl 4") << std::string("Password 1")
                           << std::string("Message 4");
    QTest::newRow("Ssl 5") << std::string("Password 1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111")
                           << std::string("Message 4");
}

void tst_Wallet::testSsl()
{
    QFETCH(std::string, password);
    QFETCH(std::string, message);

    const auto pair = createRsaKey(password);
    //qDebug() << QString::fromStdString(pair.first) << QString::fromStdString(pair.second);

    const std::string encryptedMsg = encrypt(pair.second, message);
    const std::string decryptMsg = decrypt(pair.first, password, encryptedMsg);

    QCOMPARE(decryptMsg, message);
}

void tst_Wallet::testCreateMth_data()
{
    QTest::addColumn<std::string>("passwd");

    QTest::newRow("CreateMth 1") << std::string("1");
    QTest::newRow("CreateMth 2") << std::string("123");
    QTest::newRow("CreateMth 3") << std::string("Password 1");
    QTest::newRow("CreateMth 4") << std::string("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void tst_Wallet::testCreateMth()
{
    QFETCH(std::string, passwd);
    std::string tmp;
    std::string address;
    Wallet::createWallet("./", passwd, tmp, address);
    Wallet wallet("./", address, passwd);
}

void tst_Wallet::testCreateEth_data()
{
    QTest::addColumn<std::string>("passwd");

    QTest::newRow("CreateEth 1") << std::string("1");
    QTest::newRow("CreateEth 2") << std::string("123");
    QTest::newRow("CreateEth 3") << std::string("Password 1");
    QTest::newRow("CreateEth 4") << std::string("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void tst_Wallet::testCreateEth()
{
    QFETCH(std::string, passwd);
    const std::string address = EthWallet::genPrivateKey("./", passwd);
    EthWallet wallet("./", address, passwd);
}

void tst_Wallet::testCreateBtc_data()
{
    QTest::addColumn<QString>("passwd");

    QTest::newRow("CreateBtc 0") << QStringLiteral("");
    QTest::newRow("CreateBtc 1") << QStringLiteral("1");
    QTest::newRow("CreateBtc 2") << QStringLiteral("123");
    QTest::newRow("CreateBtc 3") << QStringLiteral("Password 1");
    QTest::newRow("CreateBtc 4") << QStringLiteral("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void tst_Wallet::testCreateBtc()
{
    QFETCH(QString, passwd);
    const std::string address = BtcWallet::genPrivateKey("./", passwd).first;
    BtcWallet wallet("./", address, passwd);
    QCOMPARE(address, wallet.getAddress());
}

void tst_Wallet::testEthWallet()
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

void tst_Wallet::testBitcoinTransaction_data()
{
    QTest::addColumn<std::string>("wif");
    QTest::addColumn<std::string>("address");
    QTest::addColumn<unsigned long>("amount");
    QTest::addColumn<unsigned long>("fee");
    QTest::addColumn<QVariantList>("ins");
    QTest::addColumn<std::string>("answer");

    QTest::newRow("BitcoinTransaction 1") << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
                                            << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
                                            << 13240000UL << 10000UL
                                            << QVariantList{
                                                    QVariantList{QVariant::fromValue(std::string("f49da89eba6ef0d4935bf2edf54700710327be0bbdc5db411ad7f016e51ef922")), QVariant(0U),
                                                        QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(13250000ULL)}}
                                            << std::string("010000000122f91ee516f0d71a41dbc5bd0bbe2703710047f5edf25b93d4f06eba9ea89df4000000006a47304402203292c3b97569f90b2c4458d2b8efdeaa0fbe1f8e65840eec99bcbc626911d5f302200b5ebf256033138129563286d525145a6e7b83fc85f2cc60295290064afd89e9012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff01c006ca00000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac00000000");

    QTest::newRow("BitcoinTransaction 2") << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
                                            << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
                                            << 2000000UL << 10000UL
                                            << QVariantList{
                                                    QVariantList{QVariant::fromValue(std::string("90ecb1c712d4d9eb831d13db141726460578718382587b1ab1a3cfeaa8c472d5")), QVariant(0U),
                                                        QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(4000000ULL)}}
                                            << std::string("0100000001d572c4a8eacfa3b11a7b58828371780546261714db131d83ebd9d412c7b1ec90000000006a473044022067ac4d831d9670d63856e7ce6aa7ecfffca2dc07fb32eef7edaafe35bf9c30c402204472fc396865501cfbe9955f0f5476cd4f9f5a95b7d7658eb781dcc6873042b9012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280841e00000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac705d1e00000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000");

    QTest::newRow("BitcoinTransaction 3") << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
                                            << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
                                            << 6920000UL << 10000UL
                                            << QVariantList{
                                                    QVariantList{QVariant::fromValue(std::string("6c11ec775245041f7679d4bace0525312dc9583dede012109e8b100e6b867dce")), QVariant(0U),
                                                        QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1220000ULL)},
                                                    QVariantList{QVariant::fromValue(std::string("9f032eb20006520da112364bd91c8e0e75809aab3160fb99a78df40e783064e0")), QVariant(0U),
                                                        QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(4470000ULL)},
                                                    QVariantList{QVariant::fromValue(std::string("0afece19c37770b3be801cf8305666fc1cb2a5fd3d8989cf46a3a4298a39c3c3")), QVariant(0U),
                                                        QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1240000ULL)}}
                                            << std::string("0100000003ce7d866b0e108b9e1012e0ed3d58c92d312505cebad479761f04455277ec116c000000006b483045022100b3d988166d3b5b7c8507839f5d4e902dec8f772ccd78706caf587f5827a2863a02207ceb4611a104e455b3321a8fa4fa9829cc6241ebeacacb7c5c07d7bb56b295e1012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffe06430780ef48da799fb6031ab9a80750e8e1cd94b3612a10d520600b22e039f000000006a473044022050049602594bb1822b3c6dc1a3f0ed0c1247f108758d250e8b95dd8347b0a2cc0220707862301f0f0f7ed76a71545a6b604f95490046d6e31ba531cac3811c920090012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffc3c3398a29a4a346cf89893dfda5b21cfc665630f81c80beb37077c319cefe0a000000006b483045022100da15925c0ffc2fe2c385166399977c7bc0da8adcf78f23b1cacaa7de3e030742022007f720af79399ee9ec66ab164b7d5f026d891a175586977c3097b734dcacc35e012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0140976900000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac00000000");

    QTest::newRow("BitcoinTransaction 4") << std::string("cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG")
                                            << std::string("mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi")
                                            << 50000000UL << 10000UL
                                            << QVariantList{
                                                    QVariantList{QVariant::fromValue(std::string("72ecdaf25a178f6879c4d879551a06b2f0344ca137de3e5afb7820cdb57722b8")), QVariant(1U),
                                                        QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1990000ULL)},
                                                    QVariantList{QVariant::fromValue(std::string("94026dae0058bd0059b84e9910e5f0f30153f4b78cdeb8f1b59b54ad72bd98ca")), QVariant(0U),
                                                        QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(100000000ULL)},
                                                    QVariantList{QVariant::fromValue(std::string("0c48634b6ebf0a07430b1c08b53df81159d24902346d396bfd2d3cba2852e384")), QVariant(1U),
                                                        QVariant::fromValue(std::string("76a9145e05738474a2d065b554bd8564857e166031570688ac")), QVariant(1415000ULL)}}
                                            << std::string("0100000003b82277b5cd2078fb5a3ede37a14c34f0b2061a5579d8c479688f175af2daec72010000006b483045022100e331f87c1da0dc1f25bcd28ffd61e96339666fc5ca4729e89b2e940061fcdd6d022011748b41655046934c9e124a4ae307eb8f67a8c61b9d957e2cca0b95fcb21397012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffca98bd72ad549bb5f1b8de8cb7f45301f3f0e510994eb85900bd5800ae6d0294000000006b483045022100938db1a910e54efdf55cc5228bb7c77b5e40b19ef30eaa60ca45d40fab7f316602202bf5298ad3cfc0cb461491b31303138ad74ff61537193cb4d9b24d50a4a3a36e012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff84e35228ba3c2dfd6b396d340249d25911f83db5081c0b43070abf6e4b63480c010000006b483045022100ff7bc69634e30b614068733750e7c6e8a6906f3116f26bf9d010289bc5b185fb0220467427fd7fce3fb97f217716c00f764354ad929fb527d3b57448667b222080fc012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280f0fa02000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac38be2e03000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000");
}

void tst_Wallet::testBitcoinTransaction()
{
    QFETCH(std::string, wif);
    QFETCH(std::string, address);
    QFETCH(unsigned long, amount);
    QFETCH(unsigned long, fee);
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
    const std::string tx = wallet.genTransaction(is, amount, fee, address, true);
    QCOMPARE(tx, answer);
}

QTEST_MAIN(tst_Wallet)
