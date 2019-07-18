#include "tests.h"

#include <iostream>

#include "check.h"

#include <QFile>
#include <QTextStream>

#include "Wallet.h"
#include "EthWallet.h"
#include "BtcWallet.h"
#include "utilites/utils.h"

#include "btctx/Base58.h"

#include "openssl_wrapper/openssl_wrapper.h"

#include "ethtx/utils2.h"

#include "btctx/wif.h"

void testCreateWallet() {
    QString password = "1";
    std::string publicKey;
    std::string addr;
    const std::string exampleMessage = "Example message " + std::to_string(rand());
    std::string signature;
    Wallet::createWallet("./", false, password.toStdString(), publicKey, addr);
    publicKey.clear();
    Wallet wallet("./", false, addr, password.toStdString());
    signature = wallet.sign(exampleMessage, publicKey);
    std::cout << signature << std::endl;
}

static std::string testCreateEth(const std::string &passwd) {
    const std::string address = EthWallet::genPrivateKey("./", passwd);
    EthWallet wallet("./", address, passwd);
    //std::cout << "Ok" << std::endl;
    return address;
}

void testEthWallet(const std::string &fileName) {
    //const std::string pathToFile = "D:/qt/wallet_generator/UTC--2018-01-18T13-23-57.093123977Z--17cac1a6b8ef881cb7cf1731e7e00a7e2007cee2";
    //const std::string pathToFile = "/home/user/qtExamples/WalletMetahash/UTC--2018-01-18T13-23-57.093123977Z--17cac1a6b8ef881cb7cf1731e7e00a7e2007cee2";
    const std::string password = "0x";
    EthWallet wallet("./", fileName, password);
    const std::string result = wallet.SignTransaction(
        "0x00",
        "0x6C088E200",
        "0x8208",
        "0x8D78B1Ab426dc9daa7427b7A60E64633f62E645F",
        "0x0",
        "0x"
    );
    std::cout << result << std::endl;

    /*const std::string value("789aff1234567890abcdef");
    const std::string dump = HexStringToDump(value);
    const std::string result2 = DumpToHexString((const uint8_t*)dump.data(), dump.size());
    CHECK(result2 == value, "Incorrect " + result2);*/
}

void testBitcoinWalletCreate() {
    const auto answer = BtcWallet::genPrivateKey("./", "");
    std::cout << "Address " << answer.first << std::endl;
    std::cout << "Key " << answer.second << std::endl;
}

void testBitcoinWalletCreate2() {
    const auto answer = BtcWallet::genPrivateKey("./", "");
    std::cout << answer.first << " " << answer.second << std::endl;
}

void testBitcoinTransaction0() {
    std::vector<BtcInput> is;
    BtcInput input;

    const std::string wif = "cMfWj3UbLWnaUdZf3Be2LanryiAL5h1zDoeQA6AapPF55UMeT7gK";

    input.spendtxid = "94cbbe9c745f5c6e0057cb43e6706614629c4cb7477e494cae4422522d28f562";
    input.spendoutnum = 1;
    input.scriptPubkey = "76a914fd2aff6275d884143ef7253b7d3aaedfb046683d88ac";
    input.outBalance = 990000;
    is.push_back(input);

    BtcWallet wallet(wif);
    const std::string tx = wallet.genTransaction(is, 10000, 10000, "mjRBGbKJ6wi3G4J4WFogKAdSGygRsBKi2Z", true);
    std::cout << tx << std::endl;
}

void testBitcoinTransaction01() {
    std::vector<BtcInput> is;
    BtcInput input;

    const std::string wif = "92cPH91yyKJ87x31wqpR8NCzcVRo53airRAd4yL5UvTzio3P7hr";

    input.spendtxid = "7236f78fac62421e93980f079da6e38c20719dfc64862b9c0eeebd366dd572b3";
    input.spendoutnum = 0;
    input.scriptPubkey = "76a9140cad8759ebd814224179ec3b93344e19ea2716b488ac";
    input.outBalance = 9700;
    is.push_back(input);

    BtcWallet wallet(wif);
    const std::string tx = wallet.genTransaction(is, 9700 - 300, 300, "n4bam7UcCryKincFzRRdYKmdUAmG4mMs3D", true);
    std::cout << tx << std::endl;
}

void testCreateMetahashWallet() {
    std::string pubkey;
    std::string address;
    createFolder("./mth");
    Wallet::createWallet("./", true, "1", pubkey, address);
    std::cout << address.substr(2) << std::endl;
}

void tests2() {
    /*const std::string address = testCreateEth("0x");
    std::cout << address << std::endl;*/
    //testEthWallet("0xb15bc451d879c0C24627d9B6912fbbCA36CEBc78");
    /*for (size_t i = 0; i < 1000; i++) {
        const std::string &address = testCreateEth("0x");
        std::cout << address << std::endl;
    }*/

    //testCreateWallet();

    //testEthWalletCreate();

    /*for (size_t i = 0; i <= 10000; i++) {
        testBitcoinWalletCreate2();
    }
    return;*/

    for (size_t i = 0; i <= 1000; i++) {
        testCreateMetahashWallet();
    }
}
