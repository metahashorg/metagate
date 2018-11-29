#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include <QTcpServer>
#include <QThreadPool>

namespace proxy
{

class ProxyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ProxyServer(QObject *parent = nullptr);

    void start();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    QThreadPool *m_pool;
};

}

#endif // PROXYSERVER_H
