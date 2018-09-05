#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}


struct TestStructure
{
    QString currency;
    QString address;
    qint64 type;
    qint64 group;
    QString name;
    qreal value;
};
Q_DECLARE_METATYPE(TestStructure)

Q_DECLARE_METATYPE(QList<TestStructure>)
Q_DECLARE_METATYPE(std::vector<TestStructure>)



QDataStream &operator<<(QDataStream &out, const TestStructure &obj);
QDataStream &operator>>(QDataStream &in, TestStructure &obj);

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
