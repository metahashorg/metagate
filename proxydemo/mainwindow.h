#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "proxy/UPnPRouter.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_addMappingButton_clicked();

    void on_removeMappingButton_clicked();

private:
    void routerDiscovered(proxy::UPnPRouter *router);
private:
    Ui::MainWindow *ui;
    QList<proxy::UPnPRouter *> routers;
};

#endif // MAINWINDOW_H
