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

private:
    void routerDiscovered(proxy::UPnPRouter *router);
private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
