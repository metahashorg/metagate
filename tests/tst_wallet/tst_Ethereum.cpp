#include "tst_Ethereum.h"

#include <QTest>

#include "Wallets/EthWallet.h"
#include "Wallets/btctx/wif.h"

#include "utilites/utils.h"
#include "check.h"

#include "Wallets/openssl_wrapper/openssl_wrapper.h"

Q_DECLARE_METATYPE(std::string)

tst_Ethereum::tst_Ethereum(QObject *parent) : QObject(parent) {
    if (!isInitOpenSSL()) {
        InitOpenSSL();
    }
}

void tst_Ethereum::testCreateEth_data() {
    QTest::addColumn<std::string>("passwd");

    QTest::newRow("CreateEth 1") << std::string("1");
    QTest::newRow("CreateEth 2") << std::string("123");
    QTest::newRow("CreateEth 3") << std::string("Password 1");
    QTest::newRow("CreateEth 4") << std::string("Password 111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111111");
}

void tst_Ethereum::testCreateEth() {
    QFETCH(std::string, passwd);
    createFolder("./eth");
    const std::string address = EthWallet::genPrivateKey("./", passwd);
    EthWallet wallet("./", address, passwd);
}

void tst_Ethereum::testEthWalletTransaction()
{
    createFolder("./eth");
    writeToFile("./eth/0x05cf594f12bba9430e34060498860abc69554cb1", "{\"address\": \"05cf594f12bba9430e34060498860abc69554cb1\",\"crypto\": {\"cipher\": \"aes-128-ctr\",\"ciphertext\": \"694283a4a2f3da99186e2321c24cf1b427d81a273e7bc5c5a54ab624c8930fb8\",\"cipherparams\": {\"iv\": \"5913da2f0f6cd00b9b62ff2bc0a8b9d3\"},\"kdf\": \"scrypt\",\"kdfparams\": {\"dklen\": 32,\"n\": 262144,\"p\": 1,\"r\": 8,\"salt\": \"ca45d433267bd6a50ace149d6b317b9d8f8a39f43621bad2a3108981bf533ee7\"},\"mac\": \"0a8d581e8c60553970301603ea35b0fc56cbccd5913b12f62c690acb98d111c8\"},\"id\": \"6406896a-2ec9-4dd7-b98e-5fbfc0984e6f\",\"version\": 3}", false);
    const std::string password = "1";
    EthWallet wallet("./", "0x05cf594f12bba9430e34060498860abc69554cb1", password);
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

void tst_Ethereum::testNotCreateEthTransaction_data() {
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

void tst_Ethereum::testNotCreateEthTransaction() {
    QFETCH(std::string, to);
    QFETCH(std::string, nonce);
    QFETCH(std::string, gasPrice);
    QFETCH(std::string, gasLimit);
    QFETCH(std::string, value);
    QFETCH(std::string, data);

    createFolder("./eth");
    writeToFile("./eth/0x05cf594f12bba9430e34060498860abc69554cb1", "{\"address\": \"05cf594f12bba9430e34060498860abc69554cb1\",\"crypto\": {\"cipher\": \"aes-128-ctr\",\"ciphertext\": \"694283a4a2f3da99186e2321c24cf1b427d81a273e7bc5c5a54ab624c8930fb8\",\"cipherparams\": {\"iv\": \"5913da2f0f6cd00b9b62ff2bc0a8b9d3\"},\"kdf\": \"scrypt\",\"kdfparams\": {\"dklen\": 32,\"n\": 262144,\"p\": 1,\"r\": 8,\"salt\": \"ca45d433267bd6a50ace149d6b317b9d8f8a39f43621bad2a3108981bf533ee7\"},\"mac\": \"0a8d581e8c60553970301603ea35b0fc56cbccd5913b12f62c690acb98d111c8\"},\"id\": \"6406896a-2ec9-4dd7-b98e-5fbfc0984e6f\",\"version\": 3}", false);
    const std::string password = "1";
    EthWallet wallet("./", "0x05cf594f12bba9430e34060498860abc69554cb1", password);
    QVERIFY_EXCEPTION_THROWN(wallet.SignTransaction(
        nonce,
        gasPrice,
        gasLimit,
        to,
        value,
        data
    ), TypedException);
}

void tst_Ethereum::testHashEth_data() {
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

void tst_Ethereum::testHashEth() {
    QFETCH(std::string, transaction);
    QFETCH(std::string, answer);

    const std::string result = EthWallet::calcHash(transaction);
    QCOMPARE(result, answer);
}
