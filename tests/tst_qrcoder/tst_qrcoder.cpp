#include "tst_qrcoder.h"

#include <QTest>

#include "utilites/qrcoder.h"

tst_QRCoder::tst_QRCoder(QObject *parent)
    : QObject(parent)
{
}

void tst_QRCoder::testQRCoderEncodeDecode_data() {
    QTest::addColumn<QByteArray>("data");

    QTest::newRow("QRCoderEncodeDecode 1")
        << QByteArray::fromStdString(std::string("0009806da73b1589f38630649bdee48467946d118059efd6aab"));

    QTest::newRow("QRCoderEncodeDecode 2")
        << QByteArray(300, '\0');

    QTest::newRow("QRCoderEncodeDecode 3")
        <<  QByteArray(500, 'A');

    QTest::newRow("QRCoderEncodeDecode 4")
        << QByteArray();

    QTest::newRow("QRCoderEncodeDecode 5")
        << QByteArray("q");

    QTest::newRow("QRCoderEncodeDecode 6")
        << QByteArray::fromStdString(std::string("0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "\x00\x01\n\r"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"
                                                 "0009806da73b1589f38630649bdee48467946d118059efd6aab"));

    QTest::newRow("QRCoderEncodeDecode 7")
        << QByteArray::fromStdString(std::string("12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"
                                                 "12787328744987349849839843893434894894398"));
    QTest::newRow("QRCoderEncodeDecode 8")
        << QByteArray("0000\n\n\n\nn\nn");

    QTest::newRow("QRCoderEncodeDecode 9")
        << QByteArray("btc:L36xyLTQA4bEcFgF8aGvAcfoMBexMC3hAb25HmTBTnn8GahFVGQU 16RYK17Mwi27amr4nCR94uqWphLqm38FRY");

    QTest::newRow("QRCoderEncodeDecode 10")
        << QByteArray("tmh:f:1\nProc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,801b027ebf9b898380af57ed33594fe5\n\nFl8DM5t75F7Bucnf4b4jkf5H1zvXX1iPsxX2kp1kVRaMj7KGi//cau1oI5L9hrxT\n7yfJFMgIqTbwIi62czoMCJGBo4fh8FTK6XjRtOzPYx3dXkpaw7bIaJBP6PjXkM4D\nBIDfDdFYbuWn+WvZ//0eb2++SxKxmfDyOrW9AvJnuMs=");

    QTest::newRow("QRCoderEncodeDecode 11")
        << QByteArray("tmh:-----BEGIN EC PRIVATE KEY-----\nProc-Type: 4,ENCRYPTED\nDEK-Info: AES-128-CBC,801b027ebf9b898380af57ed33594fe5\n\nFl8DM5t75F7Bucnf4b4jkf5H1zvXX1iPsxX2kp1kVRaMj7KGi//cau1oI5L9hrxT\n7yfJFMgIqTbwIi62czoMCJGBo4fh8FTK6XjRtOzPYx3dXkpaw7bIaJBP6PjXkM4D\nBIDfDdFYbuWn+WvZ//0eb2++SxKxmfDyOrW9AvJnuMs=\n-----END EC PRIVATE KEY-----\n\n");

    QTest::newRow("QRCoderEncodeDecode 12")
        << QByteArray("eth:{\"address\": \"c951ce32add35cc55b0ca1527e96a0fe36d6c2e9\",\"crypto\": {\"cipher\": \"aes-128-ctr\",\"ciphertext\": \"4c4e86aad46b6499ef76ebe05c1f40bed39290ddfa52be52eaab61fabbb3c89e\",\"cipherparams\": {\"iv\": \"41b7cc057e6384cc10915d2b14273971\"},\"kdf\": \"scrypt\",\"kdfparams\": {\"dklen\": 32,\"n\": 262144,\"p\": 1,\"r\": 8,\"salt\": \"aae8b7784e281077fac0f54b7bd661ab2205ae17ba55c5ce8e26475fc9615f78\"},\"mac\": \"c83846fc0962ad85169e778bfb6283b7d0cfd0632ed9b813b0f32aaa37990eea\"},\"id\": \"6fcda701-217b-4b0d-b1dd-14ea14542763\",\"version\": 3}");
}

void tst_QRCoder::testQRCoderEncodeDecode() {
    QFETCH(QByteArray, data);

    QByteArray bin = QRCoder::encode(data);
    QByteArray res = QRCoder::decode(bin);
    QCOMPARE(data, res);
}

QTEST_MAIN(tst_QRCoder)
