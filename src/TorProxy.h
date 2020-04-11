#ifndef TORPROXY_H
#define TORPROXY_H

#include <QObject>

class QProcess;

namespace tor {

class TorProxy : public QObject
{
    Q_OBJECT
public:
    TorProxy(QObject *parent = nullptr);

    void start();

signals:
    void torProxyStarted(quint16 port);

private:
    void saveConfig(const QString &fn, const QString &datadir);

private:
    QProcess *torProc;
    quint16 port;
};

}

#endif // TORPROXY_H
