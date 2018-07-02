#ifndef TST_METAGATE_H
#define TST_METAGATE_H

#include <QObject>
#include <QTest>


class tst_Metagate : public QObject
{
    Q_OBJECT
public:
    explicit tst_Metagate(QObject *parent = nullptr);

private slots:

    void testEncryptBtc_data();
    void testEncryptBtc();

    void testCreateBinTransaction_data();
    void testCreateBinTransaction();

    void testSsl_data();
    void testSsl();

    void testCreateMth_data();
    void testCreateMth();

    void testCreateEth_data();
    void testCreateEth();
    
    void testCreateBtc_data();
    void testCreateBtc();

    void testEthWallet();

    void testBitcoinTransaction_data();
    void testBitcoinTransaction();
};

#endif // TST_METAGATE_H
