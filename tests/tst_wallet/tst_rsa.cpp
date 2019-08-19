#include "tst_rsa.h"

#include <QTest>

#include <iostream>

#include "utilites/utils.h"
#include "Wallets/openssl_wrapper/openssl_wrapper.h"

#include "check.h"

Q_DECLARE_METATYPE(std::string)

tst_rsa::tst_rsa(QObject *parent)
    : QObject(parent)
{
    if (!isInitOpenSSL()) {
        InitOpenSSL();
    }
}

void tst_rsa::testSsl_data() {
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

void tst_rsa::testSsl() {
    QFETCH(std::string, password);
    QFETCH(std::string, message);

    const std::string privateKey = createRsaKey(password);
    const std::string publicKey = getPublic(privateKey, password);

    const RsaKey publicKeyRsa = getPublicRsa(publicKey);
    const std::string encryptedMsg = encrypt(publicKeyRsa, message, publicKey);
    const RsaKey privateKeyRsa = getPrivateRsa(privateKey, password);
    const std::string decryptMsg = decrypt(privateKeyRsa, encryptedMsg, publicKey);

    QCOMPARE(validatePublicKey(privateKeyRsa, publicKeyRsa), true);

    QCOMPARE(decryptMsg, message);
}

void tst_rsa::testSslMessageError_data() {
    QTest::addColumn<std::string>("password");
    QTest::addColumn<std::string>("message");

    QTest::newRow("SslError 1")
        << std::string("Password 1")
        << std::string("Message 4");
}

static std::string spoilString(std::string s) {
    for (size_t i = 2; i < 10; i++) {
        const size_t pos = s.size() / i;
        if (s[pos] != '1') {
            s[pos] = '1';
        } else {
            s[pos] = '2';
        }
    }
    return s;
}

static std::string spoilStringSmall(std::string s) {
    const auto process = [](std::string &s, size_t pos) {
        if (s[pos] != '1') {
            s[pos] = '1';
        } else {
            s[pos] = '2';
        }
    };
    for (size_t i = 2; i < 3; i++) {
        const size_t pos = s.size() / i;
        process(s, pos);
    }
    process(s, s.size() - s.size() / 6);
    return s;
}

void tst_rsa::testSslMessageError() {
    QFETCH(std::string, password);
    QFETCH(std::string, message);

    const std::string privateKey = createRsaKey(password);
    const std::string publicKey = getPublic(privateKey, password);

    const RsaKey publicKeyRsa = getPublicRsa(publicKey);
    std::string encryptedMsg = encrypt(publicKeyRsa, message, publicKey);
    encryptedMsg = spoilString(encryptedMsg);
    const RsaKey privateKeyRsa = getPrivateRsa(privateKey, password);
    QVERIFY_EXCEPTION_THROWN(decrypt(privateKeyRsa, encryptedMsg, publicKey), Exception);
}

void tst_rsa::testSslWalletError_data() {
    QTest::addColumn<std::string>("password");
    QTest::addColumn<std::string>("message");

    QTest::newRow("SslError 1")
        << std::string("Password 1")
        << std::string("Message 4");
}

void tst_rsa::testSslWalletError() {
    QFETCH(std::string, password);
    QFETCH(std::string, message);

    const std::string privateKey = createRsaKey(password);
    const std::string publicKey = getPublic(privateKey, password);
    const std::string privateKeyError = spoilString(privateKey);
    const std::string pubkeyError = spoilString(publicKey);

    const RsaKey publicKeyRsa = getPublicRsa(pubkeyError);
    const std::string encryptedMsg = encrypt(publicKeyRsa, message, publicKey);
    const RsaKey privateKeyRsa = getPrivateRsa(privateKey, password);
    QVERIFY_EXCEPTION_THROWN(decrypt(privateKeyRsa, encryptedMsg, publicKey), Exception);

    QVERIFY_EXCEPTION_THROWN(getPrivateRsa(privateKeyError, password), Exception);

    QCOMPARE(validatePublicKey(privateKeyRsa, publicKeyRsa), false);
}

void tst_rsa::testSslIncorrectPassword_data() {
    QTest::addColumn<std::string>("password");
    QTest::addColumn<std::string>("message");

    QTest::newRow("SslError 1")
        << std::string("Password 1")
        << std::string("Message 4");
}

void tst_rsa::testSslIncorrectPassword() {
    QFETCH(std::string, password);
    QFETCH(std::string, message);

    const std::string privateKey = createRsaKey(password);
    const std::string errorPswd = spoilStringSmall(password);

    QVERIFY_EXCEPTION_THROWN(getPrivateRsa(privateKey, errorPswd), Exception);
}

void tst_rsa::testSslIncorrectPubkey_data() {
    QTest::addColumn<std::string>("password");
    QTest::addColumn<std::string>("message");

    QTest::newRow("SslError 1")
        << std::string("Password 1")
        << std::string("Message 4");
}

void tst_rsa::testSslIncorrectPubkey() {
    QFETCH(std::string, password);
    QFETCH(std::string, message);

    const std::string privateKey = createRsaKey(password);
    const std::string publicKey = getPublic(privateKey, password);
    const std::string privateKey2 = createRsaKey(password);
    const std::string publicKey2 = getPublic(privateKey2, password);

    const RsaKey publicKeyRsa = getPublicRsa(publicKey2);
    const std::string encryptedMsg = encrypt(publicKeyRsa, message, publicKey2);
    const RsaKey privateKeyRsa = getPrivateRsa(privateKey, password);
    QVERIFY_EXCEPTION_THROWN(decrypt(privateKeyRsa, encryptedMsg, publicKey), Exception);
}
