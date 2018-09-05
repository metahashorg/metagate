#ifndef TST_METAHASH_H
#define TST_METAHASH_H

#include <QObject>
#include <QTest>

class tst_Metahash : public QObject
{
    Q_OBJECT
public:
    explicit tst_Metahash(QObject *parent = nullptr);

private slots:

    void testCreateBinMthTransaction_data();
    void testCreateBinMthTransaction();

    void testNotCreateBinMthTransaction_data();
    void testNotCreateBinMthTransaction();

    void testCreateMth_data();
    void testCreateMth();

    void testNotCreateMth();

    void testMthSignTransaction_data();
    void testMthSignTransaction();

    void testHashMth_data();
    void testHashMth();

};

#endif // TST_METAHASH_H
