#ifndef TORPROXY_H
#define TORPROXY_H

#include <QObject>

class QProcess;

namespace tor {

class TorProxy : public QObject
{
public:
    TorProxy(QObject *parent = nullptr);

    void start();

private:
    QProcess *torProc;
};

}

#endif // TORPROXY_H
