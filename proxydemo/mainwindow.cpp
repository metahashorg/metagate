#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "proxy/UPnPDevices.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    proxy::UPnPDevices *sock = new proxy::UPnPDevices();
    connect(sock, &proxy::UPnPDevices::discovered, this, &MainWindow::routerDiscovered);
    //void discovered(bt::UPnPRouter* router);
    sock->discover();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::routerDiscovered(proxy::UPnPRouter *router)
{
    ui->listWidget->addItem(router->server());
    router->addPortMapping(8888, 8888, proxy::TCP);
    /*net::Port port;
    port.proto = net::TCP;
    port.number = 7777;
    port.forward = true;
    router->forward(port);*/
    //router->getServer();
}
