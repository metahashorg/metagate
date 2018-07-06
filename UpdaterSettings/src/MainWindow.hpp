#pragma once

#include <QMainWindow>

#include <QPointer>

class MainWindowPrivate;

/**
 * @brief The MainWindow class
 *
 * It allows to manage Clients for chat simulator
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow();
    ~MainWindow();

private:
    QScopedPointer<MainWindowPrivate> p;
};
