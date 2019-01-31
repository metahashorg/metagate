#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H

#include <QTcpSocket>
#include <memory>

namespace proxy
{

class ProxyClientPrivate;

class ProxyClient : public QTcpSocket
{
    Q_OBJECT
public:
    explicit ProxyClient(QObject *parent = nullptr);
    ~ProxyClient();

public slots:
    void stop();

private slots:
    void onSrcDisconnected();
    void onSrcError(QAbstractSocket::SocketError socketError);
    void onSrcReadyRead();
    void onDestDisconnected();
    void onDestError(QAbstractSocket::SocketError socketError);
    void onDestReadyRead();

private:
    std::unique_ptr<ProxyClientPrivate> d;
};

}

#endif // PROXYCLIENT_H
