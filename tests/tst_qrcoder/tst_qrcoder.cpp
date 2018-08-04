#include "tst_qrcoder.h"

#include "qrcoder.h"

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
}

void tst_QRCoder::testQRCoderEncodeDecode() {
    QFETCH(QByteArray, data);

    QByteArray bin = QRCoder::encode(data);
    QByteArray res = QRCoder::decode(bin);
    QCOMPARE(data, res);
}

QTEST_MAIN(tst_QRCoder)
