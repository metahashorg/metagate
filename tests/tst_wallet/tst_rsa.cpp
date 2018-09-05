#include "tst_rsa.h"

#include <QDebug>

#include <iostream>

#include "utils.h"
#include "openssl_wrapper/openssl_wrapper.h"

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
    const std::string encryptedMsg = encrypt(publicKeyRsa, message);
    const RsaKey privateKeyRsa = getPrivateRsa(privateKey, password);
    const std::string decryptMsg = decrypt(privateKeyRsa, encryptedMsg);

    QCOMPARE(decryptMsg, message);
}
