#ifndef TST_BITCOIN_H
#define TST_BITCOIN_H

#include <QObject>
#include <QTest>

class tst_Bitcoin : public QObject
{
    Q_OBJECT
public:
    explicit tst_Bitcoin(QObject *parent = nullptr);

private slots:

    void testEncryptBtc_data();
    void testEncryptBtc();

    void testReduceUtxosBtc_data();
    void testReduceUtxosBtc();

    void testHashBtc_data();
    void testHashBtc();

    void testCreateBtc_data();
    void testCreateBtc();

    void testBitcoinTransaction_data();
    void testBitcoinTransaction();

    void testNotCreateBtcTransaction_data();
    void testNotCreateBtcTransaction();

    void testNotCreateBtcTransaction2_data();
    void testNotCreateBtcTransaction2();

};

#endif // TST_BITCOIN_H
