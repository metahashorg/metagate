#ifndef TST_MESSENGERDBSTORAGE_H
#define TST_MESSENGERDBSTORAGE_H

#include <QObject>
#include <QTest>


class tst_MessengerDBStorage : public QObject
{
    Q_OBJECT
public:
    explicit tst_MessengerDBStorage(QObject *parent = nullptr);

private slots:

    void testDB();

    void testMessengerDB2();
};

#endif // TST_MESSENGERDBSTORAGE_H
