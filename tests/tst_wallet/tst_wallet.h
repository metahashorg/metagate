#ifndef TST_WALLET_H
#define TST_WALLET_H

#include <QObject>
#include <QTest>


class tst_Wallet : public QObject
{
    Q_OBJECT
public:
    explicit tst_Wallet(QObject *parent = nullptr);

private slots:

    void testEncryptBtc_data();
    void testEncryptBtc();

    void testHashBtc_data();
    void testHashBtc();

    void testCreateBinMthTransaction_data();
    void testCreateBinMthTransaction();

    void testNotCreateBinMthTransaction_data();
    void testNotCreateBinMthTransaction();

    void testSsl_data();
    void testSsl();

    void testCreateMth_data();
    void testCreateMth();

    void testNotCreateMth();

    void testMthSignTransaction_data();
    void testMthSignTransaction();

    void testCreateEth_data();
    void testCreateEth();
    
    void testCreateBtc_data();
    void testCreateBtc();

    void testEthWalletTransaction();

    void testNotCreateEthTransaction_data();
    void testNotCreateEthTransaction();

    void testBitcoinTransaction_data();
    void testBitcoinTransaction();

    void testNotCreateBtcTransaction_data();
    void testNotCreateBtcTransaction();

    void testNotCreateBtcTransaction2_data();
    void testNotCreateBtcTransaction2();

};

#endif // TST_WALLET_H
