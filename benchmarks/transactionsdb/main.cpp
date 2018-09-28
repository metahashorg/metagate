#include <QCoreApplication>
#include <QFile>
#include <QDebug>

#include <chrono>
#include <iostream>

#include "TransactionsDBStorage.h"

const QString pragmaSyncOff = "PRAGMA synchronous=OFF";
const QString pragmaSyncNormal = "PRAGMA synchronous=NORMAL";
const QString pragmaJournalWAL = "PRAGMA journal_mode=WAL";




//messenger::MessengerDBStorage *db;

using TestFunction = std::function<void(transactions::TransactionsDBStorage &)>;

void calcTime(TestFunction func, TestFunction funcp, const QStringList &pragmas = QStringList())
{
    qDebug() << "Start test";
    for (const QString &sql: pragmas)
        qDebug() << sql;
    const int nmax = 20;
    qreal time = 0.0;
    for (int n = 0; n < nmax; n++) {
        if (QFile::exists("messenger.db"))
            QFile::remove("messenger.db");
        transactions::TransactionsDBStorage db;
        db.init();
        for (const QString &sql: pragmas)
            db.execPragma(sql);

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        func(db);
        std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();
        auto d = std::chrono::duration_cast<std::chrono::microseconds>(end - begin).count();
        //qDebug() << d;
        time += d;
    }
    time /= nmax;
    time /= 1000000.0;

    qDebug() << QString::number(time, 'f', 6) << "s";
}

void emptyInit(transactions::TransactionsDBStorage &)
{
}

void insert2000Transactions(transactions::TransactionsDBStorage &db)
{
    auto transactionGuard = db.beginTransaction();
    for (qint64 n = 0; n < 3000; n++) {
        db.addPayment("mh", QString("gfklklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address100", true, "user7", "user1", "9000000000000000000", 1000 + 2 * n, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    }
    transactionGuard.commit();
}

void insert3000Transactions(transactions::TransactionsDBStorage &db)
{
    for (qint64 n = 0; n < 1; n++) {
        db.addPayment("mh", QString("gfklklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address100", true, "user7", "user1", "9000000000000000000", 1000 + 2 * n, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    }
}

void insert3000TransactionsV2(transactions::TransactionsDBStorage &db)
{
    for (qint64 n = 0; n < 1; n++) {
        db.addPayment("mh", QString("gfklklkltrklklgfmjgfhg%1").arg(QString::number(n)), "address100", true, "user7", "user1", "9000000000000000000", 1000 + 2 * n, "nvcmnjkdfjkgf", "100", 8896865, false, false, "100");
    }
}


void insert2000TransactionsV(transactions::TransactionsDBStorage &db)
{
    std::vector<transactions::Transaction> transactions;
    transactions.reserve(3100);
    for (qint64 n = 0; n < 3000; n++) {
        transactions::Transaction trans;
        trans.currency = "mh";
        trans.tx = QString("gfklklkltrklklgfmjgfhg%1").arg(QString::number(n));
        trans.address = "address100";
        trans.isInput = true;
        trans.from = "user7";
        trans.to = "user1";
        trans.value = "9000000000000000000";
        trans.timestamp = 1000 + 2 * n;
        trans.data = "nvcmnjkdfjkgf";
        trans.fee = "100";
        trans.nonce = 8896865;
        trans.isSetDelegate = false;
        trans.isDelegate = false;
        trans.delegateValue = "100";
        transactions.push_back(trans);
    }
    db.addPayments(transactions);
}

void selectTransactions(transactions::TransactionsDBStorage &db)
{
    std::vector<transactions::Transaction> res = db.getPaymentsForAddress("address100", "mh", 55, 1000, true);
    //qDebug() << res.size();
}

int main(int argc, char *argv[])
{
    //QCoreApplication a(argc, argv);

    qDebug() << "Inserts 3000 transactions";
    calcTime(insert3000Transactions, emptyInit, QStringList{pragmaSyncNormal, pragmaJournalWAL});
    qDebug() << "Inserts 3000 transactions V2";
    calcTime(insert3000TransactionsV2, emptyInit, QStringList{pragmaSyncNormal, pragmaJournalWAL});
    //calcTime(insert3000Transactions, emptyInit);

    /*qDebug() << "Insert 2000 transactions";
    calcTime(insert2000Transactions, emptyInit);
    calcTime(insert2000Transactions, emptyInit, QStringList{pragmaSyncNormal});
    calcTime(insert2000Transactions, emptyInit, QStringList{pragmaSyncNormal, pragmaJournalWAL});
    calcTime(insert2000Transactions, emptyInit, QStringList{pragmaJournalWAL});

    qDebug() << "Insert 2000 transactions Vector";
    calcTime(insert2000TransactionsV, emptyInit);
    calcTime(insert2000TransactionsV, emptyInit, QStringList{pragmaSyncNormal});
    calcTime(insert2000TransactionsV, emptyInit, QStringList{pragmaSyncNormal, pragmaJournalWAL});
    calcTime(insert2000TransactionsV, emptyInit, QStringList{pragmaJournalWAL});


    calcTime(selectTransactions, insert2000TransactionsV, QStringList{pragmaSyncNormal, pragmaJournalWAL});*/
/*
    {

        std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        QFile file("out.txt");
        if (file.open(QIODevice::WriteOnly)) {
            QByteArray b(65536, 'A');
            file.write(b);
            file.flush();
            file.close();
        }
        std::chrono::steady_clock::time_point end= std::chrono::steady_clock::now();

        std::cout << "Time = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() <<std::endl;

    }*/
    qDebug() << "ok";

    return 0;
    //return a.exec();
}
