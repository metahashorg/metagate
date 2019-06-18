#ifndef TST_MESSENGERDBSTORAGE_H
#define TST_MESSENGERDBSTORAGE_H

#include <QObject>

class tst_TransactionsDBStorage : public QObject
{
    Q_OBJECT
public:
    explicit tst_TransactionsDBStorage(QObject *parent = nullptr);

private slots:

    void testDB1();
    void testBigNumSum();
    void testGetPayments();
    void testAddressInfos();
    void testBlockNumer();

    void tstFilterDelegate();

private:
};

#endif // TST_MESSENGERDBSTORAGE_H
