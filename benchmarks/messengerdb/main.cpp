#include <QCoreApplication>
#include <QFile>
#include <QDebug>

#include <chrono>
#include <iostream>

#include "MessengerDBStorage.h"

const QString pragmaSyncOff = "PRAGMA synchronous=OFF";
const QString pragmaSyncNormal = "PRAGMA synchronous=NORMAL";
const QString pragmaJournalWAL = "PRAGMA journal_mode=WAL";




//messenger::MessengerDBStorage *db;

using TestFunction = std::function<void(messenger::MessengerDBStorage &)>;

void calcTime(TestFunction func, const QStringList &pragmas = QStringList())
{
    qDebug() << "Start test";
    for (const QString &sql: pragmas)
        qDebug() << sql;
    const int nmax = 20;
    qreal time = 0.0;
    for (int n = 0; n < nmax; n++) {
        if (QFile::exists("messenger.db"))
            QFile::remove("messenger.db");
        messenger::MessengerDBStorage db;
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

void insert1Message(messenger::MessengerDBStorage &db)
{
    db.addMessage("1234", "3454", "abcd", 1000000, 4001, true, true, true, "asdfdf", 1);
}

void insert1MessageTrans(messenger::MessengerDBStorage &db)
{
    auto transactionGuard = db.beginTransaction();
    db.addMessage("1234", "3454", "abcd", 1000000, 4001, true, true, true, "asdfdf", 1);
    transactionGuard.commit();
}

void insert1000Messages(messenger::MessengerDBStorage &db)
{
    auto transactionGuard = db.beginTransaction();
    for (int n = 0; n < 1000; n++) {
        db.addMessage("1234", "3454", "abcd", 1000000 + n, 4001 + n, true, true, true, "asdfdf", 1);
    }
    transactionGuard.commit();
}

int main(int argc, char *argv[])
{
    //QCoreApplication a(argc, argv);

    qDebug() << "Insert 1000 messages";
    calcTime(insert1000Messages);
    calcTime(insert1000Messages, QStringList{pragmaSyncOff});
    calcTime(insert1000Messages, QStringList{pragmaSyncNormal});
    calcTime(insert1000Messages, QStringList{pragmaSyncNormal, pragmaJournalWAL});
    calcTime(insert1000Messages, QStringList{pragmaJournalWAL});

    /*
    calcTime(insert1Message);
    calcTime(insert1Message, QStringList{pragmaSyncOff});
    calcTime(insert1Message, QStringList{pragmaSyncNormal});
    calcTime(insert1Message, QStringList{pragmaSyncNormal, pragmaJournalWAL});
    calcTime(insert1Message, QStringList{pragmaJournalWAL});
    */

    qDebug() << "Insert one message";
    calcTime(insert1MessageTrans);
    calcTime(insert1MessageTrans, QStringList{pragmaSyncOff});
    calcTime(insert1MessageTrans, QStringList{pragmaSyncNormal});
    calcTime(insert1MessageTrans, QStringList{pragmaSyncNormal, pragmaJournalWAL});
    calcTime(insert1MessageTrans, QStringList{pragmaJournalWAL});


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
