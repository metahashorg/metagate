#ifndef TST_WALLETNAMESDBSTORAGE_H
#define TST_WALLETNAMESDBSTORAGE_H

#include <QObject>

class tst_WalletNamesDBStorage : public QObject
{
    Q_OBJECT
public:
    explicit tst_WalletNamesDBStorage(QObject *parent = nullptr);

private slots:

    void testGiveName();

    void testUpdateInfo();

    void testRenameWallet();

    void testSelectForCurrency();

private:
};

#endif // TST_WALLETNAMESDBSTORAGE_H
