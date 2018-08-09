#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QTreeWidget>

#include "dbstorage.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    DBStorage::instance()->init();

    DBStorage::instance()->addPayment("818af8b3a50a5b6758360d11a44533f596af76feb9b4e57557220a8536c38165",
                                      "818af8b3a50a5b6758360d11a44533f596af76feb9b4e57557220a8536c38165",
                                      "1CKZ3fJrojYauQHUZiuSKeuvw1AwBVUwHX",
                                      "19BWPGPJM9KAG6r8vRsLH9e6w495dx3C5W,1",
                                      "0.0006650",
                                      "66502",
                                      533227,
                                      true,
                                      1532329669,
                                      "7");


    QList<QStringList> r = DBStorage::instance()->getPayments();
    foreach(const QStringList &l, r) {
        QTreeWidgetItem *item = new QTreeWidgetItem(ui->treeWidget);
        for (int c = 0; c < 10; c++)
            item->setText(c, l.value(c));
    }

    /*
    QSqlTableModel *model = new QSqlTableModel(parentObject, database);
    model->setTable("employee");
    model->setEditStrategy(QSqlTableModel::OnManualSubmit);
    model->select();
    model->setHeaderData(0, Qt::Horizontal, tr("Name"));
    model->setHeaderData(1, Qt::Horizontal, tr("Salary"));

    QTableView *view = new QTableView;
    view->setModel(model);
    view->hideColumn(0); // don't show the ID
    view->show();*/
}

MainWindow::~MainWindow()
{
    delete ui;
}
