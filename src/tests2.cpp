#include "tests2.h"

#include <iostream>

#include "check.h"

#include <QFile>
#include <QTextStream>

#include "Wallet.h"
#include "EthWallet.h"
#include "BtcWallet.h"
#include "utils.h"

#include "btctx/Base58.h"

#include "openssl_wrapper/openssl_wrapper.h"

#include "ethtx/utils2.h"

#include "btctx/wif.h"

static void testSsl(const std::string &password, const std::string &message) {
    const auto pair = createRsaKey(password);
    //std::cout << pair.first << "\n" << pair.second << std::endl;

    const std::string encryptedMsg = encrypt(pair.second, message);
    //std::cout << encryptedMsg << std::endl;

    const std::string decryptMsg = decrypt(pair.first, password, encryptedMsg);
    CHECK(decryptMsg == message, "Ups " + decryptMsg);
    std::cout << "Ok" << std::endl;
}

static void testEncryptBtc() {
    const std::string result = encryptWif("5KN7MzqK5wt2TP1fQCYyHBtDrXdJuXbUzm4A9rKAteGu3Qi5CVR", QString("TestingOneTwoThree").normalized(QString::NormalizationForm_C).toStdString());
    CHECK(result == "6PRVWUbkzzsbcVac2qwfssoUJAN1Xhrg6bNk8J7Nzm5H7kxEbn2Nh2ZoGg", "Incorrect result: " + result);
    const std::string resultDecode = decryptWif(result, QString("TestingOneTwoThree").normalized(QString::NormalizationForm_C).toStdString());
    CHECK(resultDecode == "5KN7MzqK5wt2TP1fQCYyHBtDrXdJuXbUzm4A9rKAteGu3Qi5CVR", "Incorrect result: " + resultDecode);

    const std::string result2 = encryptWif("L44B5gGEpqEDRS9vVPz7QT35jcBG2r3CZwSwQ4fCewXAhAhqGVpP", QString("TestingOneTwoThree").normalized(QString::NormalizationForm_C).toStdString());
    CHECK(result2 == "6PYNKZ1EAgYgmQfmNVamxyXVWHzK5s6DGhwP4J5o44cvXdoY7sRzhtpUeo", "Incorrect result: " + result2)
    const std::string resultDecode2 = decryptWif(result2, QString("TestingOneTwoThree").normalized(QString::NormalizationForm_C).toStdString());
    CHECK(resultDecode2 == "L44B5gGEpqEDRS9vVPz7QT35jcBG2r3CZwSwQ4fCewXAhAhqGVpP", "Incorrect result: " + resultDecode2);

#ifndef TARGET_WINDOWS
    QString passwd3 = QString::fromStdString(std::string("\u03D2\u0301\u0000\U00010400\U0001F4A9", 13));
    const std::string normalizedPasswd = passwd3.normalized(QString::NormalizationForm_C, QChar::Unicode_2_0).toStdString();
    CHECK(toHex(normalizedPasswd) == "cf9300f0909080f09f92a9", "Incorrect normalization " + toHex(normalizedPasswd));
    const std::string result3 = encryptWif("5Jajm8eQ22H3pGWLEVCXyvND8dQZhiQhoLJNKjYXk9roUFTMSZ4", passwd3.normalized(QString::NormalizationForm_C).toStdString());
    CHECK(result3 == "6PRW5o9FLp4gJDDVqJQKJFTpMvdsSGJxMYHtHaQBF3ooa8mwD69bapcDQn", "Incorrect result: " + result3)
    const std::string resultDecode3 = decryptWif(result3, passwd3.normalized(QString::NormalizationForm_C).toStdString());
    CHECK(resultDecode3 == "5Jajm8eQ22H3pGWLEVCXyvND8dQZhiQhoLJNKjYXk9roUFTMSZ4", "Incorrect result: " + resultDecode3);
#endif
    std::cout << "Ok" << std::endl;
}

static void testCreateMth(const std::string &passwd) {
    std::string tmp;
    std::string address;
    Wallet::createWallet("./", passwd, tmp, address);
    Wallet wallet("./", address, passwd);
    std::cout << "Ok" << std::endl;
}

static void testCreateEth(const std::string &passwd) {
    const std::string address = EthWallet::genPrivateKey("./", passwd);
    EthWallet wallet("./", address, passwd);
    std::cout << "Ok" << std::endl;
}

static void testCreateBtc(const QString &passwd) {
    const std::string address = BtcWallet::genPrivateKey("./", passwd).first;
    BtcWallet wallet("./", address, passwd);
    CHECK(address == wallet.getAddress(), "Incorrect address");
    std::cout << "Ok" << std::endl;
}

static void testEthWallet() {
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
    CHECK(result == "0xf899018506c088e200828208948d78b1ab426dc9daa7427b7a60e64633f62e645f85746a528800b001010101010101010101010101010101010101010101010101010101010101010101010101010101010101010101010126a047dd9f6ebce749230df9ac9d57db85f948db0775882cb63565501fe95ddfcb58a07c7020426395bc781fc06e4fbb5cffc5c4d8b77d37596b1c83fa0c21ce37cfb3", "Incorrect result: " + result);
}

static void testBitcoinTransaction() {
    std::vector<BtcInput> is;
    BtcInput input;

    const std::string wif = "cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG";

    input.spendtxid = "f49da89eba6ef0d4935bf2edf54700710327be0bbdc5db411ad7f016e51ef922";
    input.spendoutnum = 0;
    input.scriptPubkey = "76a9145e05738474a2d065b554bd8564857e166031570688ac";
    input.outBalance = 13250000;
    is.push_back(input);

    BtcWallet wallet(wif);
    const std::string tx = wallet.genTransaction(is, 13240000, 10000, "mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi", true);
    if (tx != "010000000122f91ee516f0d71a41dbc5bd0bbe2703710047f5edf25b93d4f06eba9ea89df4000000006a47304402203292c3b97569f90b2c4458d2b8efdeaa0fbe1f8e65840eec99bcbc626911d5f302200b5ebf256033138129563286d525145a6e7b83fc85f2cc60295290064afd89e9012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff01c006ca00000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac00000000") {
        std::cout << "Not ok 1" << std::endl;
    } else {
        std::cout << "Ok" << std::endl;
    }
}

static void testBitcoinTransaction2() {
    std::vector<BtcInput> is;
    BtcInput input;

    const std::string wif = "cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG";

    input.spendtxid = "90ecb1c712d4d9eb831d13db141726460578718382587b1ab1a3cfeaa8c472d5";
    input.spendoutnum = 0;
    input.scriptPubkey = "76a9145e05738474a2d065b554bd8564857e166031570688ac";
    input.outBalance = 4000000;
    is.push_back(input);

    BtcWallet wallet(wif);
    const std::string tx = wallet.genTransaction(is, 2000000, 10000, "mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi", true);
    if (tx != "0100000001d572c4a8eacfa3b11a7b58828371780546261714db131d83ebd9d412c7b1ec90000000006a473044022067ac4d831d9670d63856e7ce6aa7ecfffca2dc07fb32eef7edaafe35bf9c30c402204472fc396865501cfbe9955f0f5476cd4f9f5a95b7d7658eb781dcc6873042b9012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280841e00000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac705d1e00000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000") {
        std::cout << "Not ok 1" << std::endl;
    } else {
        std::cout << "Ok" << std::endl;
    }
}

static void testBitcoinTransaction3() {
    std::vector<BtcInput> is;
    BtcInput input;

    const std::string wif = "cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG";

    input.spendtxid = "6c11ec775245041f7679d4bace0525312dc9583dede012109e8b100e6b867dce";
    input.spendoutnum = 0;
    input.scriptPubkey = "76a9145e05738474a2d065b554bd8564857e166031570688ac";
    input.outBalance = 1220000;
    is.push_back(input);

    input.spendtxid = "9f032eb20006520da112364bd91c8e0e75809aab3160fb99a78df40e783064e0";
    input.spendoutnum = 0;
    input.scriptPubkey = "76a9145e05738474a2d065b554bd8564857e166031570688ac";
    input.outBalance = 4470000;
    is.push_back(input);

    input.spendtxid = "0afece19c37770b3be801cf8305666fc1cb2a5fd3d8989cf46a3a4298a39c3c3";
    input.spendoutnum = 0;
    input.scriptPubkey = "76a9145e05738474a2d065b554bd8564857e166031570688ac";
    input.outBalance = 1240000;
    is.push_back(input);

    BtcWallet wallet(wif);
    const std::string tx = wallet.genTransaction(is, 6920000, 10000, "mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi", true);
    if (tx != "0100000003ce7d866b0e108b9e1012e0ed3d58c92d312505cebad479761f04455277ec116c000000006b483045022100b3d988166d3b5b7c8507839f5d4e902dec8f772ccd78706caf587f5827a2863a02207ceb4611a104e455b3321a8fa4fa9829cc6241ebeacacb7c5c07d7bb56b295e1012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffe06430780ef48da799fb6031ab9a80750e8e1cd94b3612a10d520600b22e039f000000006a473044022050049602594bb1822b3c6dc1a3f0ed0c1247f108758d250e8b95dd8347b0a2cc0220707862301f0f0f7ed76a71545a6b604f95490046d6e31ba531cac3811c920090012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffc3c3398a29a4a346cf89893dfda5b21cfc665630f81c80beb37077c319cefe0a000000006b483045022100da15925c0ffc2fe2c385166399977c7bc0da8adcf78f23b1cacaa7de3e030742022007f720af79399ee9ec66ab164b7d5f026d891a175586977c3097b734dcacc35e012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0140976900000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac00000000") {
        std::cout << "Not ok 1" << std::endl;
    } else {
        std::cout << "Ok" << std::endl;
    }
}

static void testBitcoinTransaction4() {
    std::vector<BtcInput> is;
    BtcInput input;

    const std::string wif = "cUzkK2uj56xSuwY2Ha9TMjKgwPr1uBwNKXbSB3eGbcSbZ77YwQRG";

    input.spendtxid = "72ecdaf25a178f6879c4d879551a06b2f0344ca137de3e5afb7820cdb57722b8";
    input.spendoutnum = 1;
    input.scriptPubkey = "76a9145e05738474a2d065b554bd8564857e166031570688ac";
    input.outBalance = 1990000;
    is.push_back(input);

    input.spendtxid = "94026dae0058bd0059b84e9910e5f0f30153f4b78cdeb8f1b59b54ad72bd98ca";
    input.spendoutnum = 0;
    input.scriptPubkey = "76a9145e05738474a2d065b554bd8564857e166031570688ac";
    input.outBalance = 100000000;
    is.push_back(input);

    input.spendtxid = "0c48634b6ebf0a07430b1c08b53df81159d24902346d396bfd2d3cba2852e384";
    input.spendoutnum = 1;
    input.scriptPubkey = "76a9145e05738474a2d065b554bd8564857e166031570688ac";
    input.outBalance = 1415000;
    is.push_back(input);

    BtcWallet wallet(wif);
    const std::string tx = wallet.genTransaction(is, 50000000, 10000, "mkDQ29a4WtweYxagdhwuTx8P6BtsTnkJwi", true);
    if (tx != "0100000003b82277b5cd2078fb5a3ede37a14c34f0b2061a5579d8c479688f175af2daec72010000006b483045022100e331f87c1da0dc1f25bcd28ffd61e96339666fc5ca4729e89b2e940061fcdd6d022011748b41655046934c9e124a4ae307eb8f67a8c61b9d957e2cca0b95fcb21397012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffffca98bd72ad549bb5f1b8de8cb7f45301f3f0e510994eb85900bd5800ae6d0294000000006b483045022100938db1a910e54efdf55cc5228bb7c77b5e40b19ef30eaa60ca45d40fab7f316602202bf5298ad3cfc0cb461491b31303138ad74ff61537193cb4d9b24d50a4a3a36e012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff84e35228ba3c2dfd6b396d340249d25911f83db5081c0b43070abf6e4b63480c010000006b483045022100ff7bc69634e30b614068733750e7c6e8a6906f3116f26bf9d010289bc5b185fb0220467427fd7fce3fb97f217716c00f764354ad929fb527d3b57448667b222080fc012102ccb646cc5cc5fcb76e8ff0576366c71dd729f8395f25e863215e44d8d344a907ffffffff0280f0fa02000000001976a91433869dcc29235cd6d3369de263f1ab54463ee65688ac38be2e03000000001976a9145e05738474a2d065b554bd8564857e166031570688ac00000000") {
        std::cout << "Not ok 1" << std::endl;
    } else {
        std::cout << "Ok" << std::endl;
    }
}

static void testCreateBinTransaction(const std::string &address, uint64_t amount, uint64_t fee, uint64_t nonce, const std::string &answer) {
    const std::string result = toHex(Wallet::genTx(address, amount, fee, nonce, ""));
    if (result == answer) {
        std::cout << "Ok" << std::endl;
    } else {
        std::cout << "Not ok bin " << result << std::endl;
    }
}

void allTests() {
    testEncryptBtc();

    testCreateBinTransaction("0x009806da73b1589f38630649bdee48467946d118059efd6aab", 126894, 55647, 255, "009806da73b1589f38630649bdee48467946d118059efd6aabfbaeef0100fa5fd9faff0000");
    testCreateBinTransaction("0x009806da73b1589f38630649bdee48467946d118059efd6aab", 0, 0, 0, "009806da73b1589f38630649bdee48467946d118059efd6aab00000000");
    testCreateBinTransaction("0x009806da73b1589f38630649bdee48467946d118059efd6aab", 4294967295, 65535, 249, "009806da73b1589f38630649bdee48467946d118059efd6aabfbfffffffffafffff900");
    testCreateBinTransaction("0x009806da73b1589f38630649bdee48467946d118059efd6aab", 4294967296ull, 65536, 250, "009806da73b1589f38630649bdee48467946d118059efd6aabfc0000000001000000fb00000100fafa0000");

    testSsl("", "Message 1");
    testSsl("1", "Message 2");
    testSsl("123", "Message 3");
    testSsl("Password 1", "Message 4");
    testSsl("Password 1111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111", "Message 4");

    //testCreateMth("");
    testCreateMth("1");
    testCreateMth("123");
    testCreateMth("Password 1");
    testCreateMth("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");

    //testCreateEth("");
    testCreateEth("1");
    testCreateEth("123");
    testCreateEth("Password 1");
    testCreateEth("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");

    testCreateBtc("");
    testCreateBtc("1");
    testCreateBtc("123");
    testCreateBtc("Password 1");
    testCreateBtc("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");

    testEthWallet();

    testBitcoinTransaction();
    testBitcoinTransaction2();
    testBitcoinTransaction3();
    testBitcoinTransaction4();
}
