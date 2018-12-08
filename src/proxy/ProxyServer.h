#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include <QTcpServer>

namespace proxy
{

class ProxyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ProxyServer(QObject *parent = nullptr);

    quint16 port() const;
    void setPort(quint16 p);

    void start();
    void stop();

signals:
    void listeningChanged(bool s);

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    quint16 m_port;
};

}

#endif // PROXYSERVER_H
