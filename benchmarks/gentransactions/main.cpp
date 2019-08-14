#include <QCoreApplication>
#include <QFile>
#include <QDebug>

#include <chrono>
#include <iostream>

#include "TransactionsDBStorage.h"
#include "SlotWrapper.h"

#include <openssl/evp.h>
#include <openssl/ec.h>
#include <openssl/bio.h>
#include <openssl/x509.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
using MHKeysPair = QPair<QByteArray, QByteArray>;
QStringList addresses;

MHKeysPair createKeysPair()
{
    // EC parameters
    const point_conversion_form_t form = POINT_CONVERSION_UNCOMPRESSED;
    const int asn1_flag = OPENSSL_EC_NAMED_CURVE;
    const int nid = NID_secp256k1;

    std::unique_ptr<EC_GROUP, std::function<void(EC_GROUP *)>> group(EC_GROUP_new_by_curve_name(nid), EC_GROUP_free);
    if (!group.get()) {
        qWarning() << "Unable to create curve";
        return MHKeysPair();
    }
    EC_GROUP_set_asn1_flag(group.get(), asn1_flag);
    EC_GROUP_set_point_conversion_form(group.get(), form);

    // Create keys pair
    std::unique_ptr<EC_KEY, std::function<void(EC_KEY *)>> eckey(EC_KEY_new(), EC_KEY_free);
    if (!eckey.get()) {
        qWarning() << "Unable to create key";
        return MHKeysPair();
    }

    if (EC_KEY_set_group(eckey.get(), group.get()) == 0) {
        qWarning() << "Unable set group";
        return MHKeysPair();
    }


    if (!EC_KEY_generate_key(eckey.get())) {
        qWarning() <<  "Unable to generate key";
        return MHKeysPair();
    }

    unsigned char *prKeyBuffer = NULL;
    int prKeyLen = i2d_ECPrivateKey(eckey.get(),  &prKeyBuffer);
    if (prKeyLen <= 0) {
        if (prKeyBuffer)
          free(prKeyBuffer);
        qWarning() << "Unable to store private key";
        return MHKeysPair();
    }
    QByteArray prKey(reinterpret_cast<const char *>(prKeyBuffer), prKeyLen);
    //qDebug() << prKey.toHex();

    unsigned char *pubKeyBuffer = NULL;
    int pubKeyLen = i2d_EC_PUBKEY(eckey.get(),  &pubKeyBuffer);
    if (pubKeyLen <= 0) {
        if (prKeyBuffer)
          free(prKeyBuffer);
        if (pubKeyBuffer)
          free(pubKeyBuffer);
        qWarning() << "Unable to store public key";
        return MHKeysPair();
    }
    QByteArray pubKey(reinterpret_cast<const char *>(pubKeyBuffer), pubKeyLen);
    //qDebug() << pubKey.toHex();

    if (prKeyBuffer)
      free(prKeyBuffer);
    if (pubKeyBuffer)
      free(pubKeyBuffer);
    return MHKeysPair(prKey, pubKey);
}

QByteArray createAddress(const QByteArray &key)
{
    // Use last 65 bytes for public key
    QByteArray tkey = key.right(65);

    unsigned char sha256buf[SHA256_DIGEST_LENGTH];
    unsigned char ripemd160buf[RIPEMD160_DIGEST_LENGTH + 1];

    // Use SHA-256 first
    SHA256(reinterpret_cast<const unsigned char *>(tkey.data()), tkey.size(), sha256buf);

    // Use RIPEMD-160 for previous result and add 0x00 byte to the begin
    RIPEMD160(sha256buf, SHA256_DIGEST_LENGTH, ripemd160buf + 1);
    ripemd160buf[0] = 0;

    // Do SHA-256 for previous result twice
    SHA256(ripemd160buf, RIPEMD160_DIGEST_LENGTH + 1, sha256buf);
    SHA256(sha256buf, SHA256_DIGEST_LENGTH, sha256buf);

    // Build address
    QByteArray data = QByteArray (RIPEMD160_DIGEST_LENGTH + 5, ' ');
    memcpy(data.data(), ripemd160buf, RIPEMD160_DIGEST_LENGTH + 1);
    memcpy(data.data() + RIPEMD160_DIGEST_LENGTH + 1, sha256buf, 4);
    return data;
}

void writeWallet(const QString &addr)
{
    QFile file(addr + QStringLiteral(".watch"));
    if (!file.open(QIODevice::WriteOnly))
        return;
    file.close();
}

void genAddresses()
{
    for (qint64 n = 0; n < 100; n++) {
        MHKeysPair keys = createKeysPair();
        QString addr = QStringLiteral("0x") + createAddress(keys.second).toHex();
        qDebug() << addr;
        writeWallet(addr);
        addresses << addr;
    }
}

void insertTransactionsV2(transactions::TransactionsDBStorage &db)
{
    int s = addresses.size();
    auto transactionGuard = db.beginTransaction();
    for (qint64 n = 0; n < 1000000; n++) {
        int r = n % (s * s);
        int from = r % s;
        int to = r / s;
        QString addr = (n / 2) ? addresses[from] : addresses[to];
        db.addPayment("tmh", QString("8e0c32f784a2e6a1bf9a90ebb54b3d795dc3a1ae443758f0d98ab5218f13bafd"), addr, 7,
                  addresses[from], addresses[to],
                  "1000000", 1556011985 + 2 * n, "nvcmnjkdfjkgf", "0", 7,
                  false, "", "",
                  transactions::Transaction::OK, transactions::Transaction::SIMPLE, 78775 + n,
                  "", 0);
    }
    transactionGuard.commit();

}
void calcTime()
{
    qDebug() << "Start";
    {
        //if (QFile::exists("payments.db"))
        //    QFile::remove("payments.db");
        transactions::TransactionsDBStorage db;
        db.init();
        insertTransactionsV2(db);
    }

}


int main(int argc, char *argv[])
{
    genAddresses();
    calcTime();
    qDebug() << "ok";

    return 0;
    //return a.exec();
}
