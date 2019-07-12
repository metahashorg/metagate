#ifndef TST_BIGNUMBER_H
#define TST_BIGNUMBER_H

#include <QObject>

class tst_BigNumber : public QObject
{
    Q_OBJECT
public:
    explicit tst_BigNumber(QObject *parent = nullptr);

private slots:

    void testBigNumberDecimal_data();
    void testBigNumberDecimal();

    void testBigNumberSum_data();
    void testBigNumberSum();

    void testBigNumberSub_data();
    void testBigNumberSub();

    void testBigNumberFracDecimal_data();
    void testBigNumberFracDecimal();
};

#endif // TST_BIGNUMBER_H
