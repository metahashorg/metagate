#ifndef MHPAYEVENTHANDLER_H
#define MHPAYEVENTHANDLER_H

#include <QObject>
#include <QUrl>
#include <QTimer>

class MainWindow;
class RunGuard;

class MhPayEventHandler: public QObject {
    Q_OBJECT
public:

    MhPayEventHandler(RunGuard &runGuard);

    void setMainWindow(MainWindow *mw);

public:

    void processCommandLine(const QString &arg);

protected:

    bool eventFilter(QObject* object, QEvent* event);

private:

    bool processUrl(const QUrl &url);

private slots:

    void onTimerEvent();

private:

    RunGuard &runGuard;

    QUrl lastUrl;

    MainWindow *mainWindow = nullptr;

    QTimer timer;
};

#endif // MHPAYEVENTHANDLER_H
