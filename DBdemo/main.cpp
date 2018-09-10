#include "mainwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    qRegisterMetaType<TestStructure>();
    qRegisterMetaTypeStreamOperators<TestStructure>();
    qRegisterMetaType<QList<TestStructure>>();
    qRegisterMetaTypeStreamOperators<QList<TestStructure>>();

    QApplication a(argc, argv);
    MainWindow w;
    w.show();

    return a.exec();
}
