#ifndef PROXYSERVER_H
#define PROXYSERVER_H

#include <QTcpServer>

namespace proxy
{

class ProxyClient;

class ProxyServer : public QTcpServer
{
    Q_OBJECT
public:
    explicit ProxyServer(QObject *parent = nullptr);

    quint16 port() const;
    void setPort(quint16 p);

    bool start();
    void stop();

signals:
    void listeningChanged(bool s);
    void stopClient();

protected:
    void incomingConnection(qintptr socketDescriptor) override;

private:
    quint16 m_port;
    QList<ProxyClient *> clients;
};

}

#endif // PROXYSERVER_H
