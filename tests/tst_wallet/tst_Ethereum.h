#ifndef TST_ETHEREUM_H
#define TST_ETHEREUM_H

#include <QObject>

class tst_Ethereum : public QObject {
    Q_OBJECT
public:
    explicit tst_Ethereum(QObject *parent = nullptr);

private slots:

    void testHashEth_data();
    void testHashEth();

    void testCreateEth_data();
    void testCreateEth();

    void testEthWalletTransaction();

    void testNotCreateEthTransaction_data();
    void testNotCreateEthTransaction();

};

#endif // TST_ETHEREUM_H
