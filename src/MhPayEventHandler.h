#ifndef MHPAYEVENTHANDLER_H
#define MHPAYEVENTHANDLER_H

#include <QObject>
#include <QUrl>

class MainWindow;

class MhPayEventHandler: public QObject {
    Q_OBJECT
public:

    void setMainWindow(MainWindow *mw);

protected:
    bool eventFilter(QObject* object, QEvent* event);

private:

    QUrl lastUrl;

    MainWindow *mainWindow = nullptr;

};

#endif // MHPAYEVENTHANDLER_H
