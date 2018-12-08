#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H

#include <QTcpSocket>
#include <QNetworkAccessManager>

namespace proxy
{

class ProxyClientPrivate;

class ProxyClient : public QTcpSocket
{
    Q_OBJECT
public:
    explicit ProxyClient(QObject *parent = nullptr);

    virtual bool event(QEvent *e) override;

    void sendResponse(const QByteArray &d);

private slots:
    void onConnected();
    void onDisconnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();

private:
    void parseResp(const QByteArray &data);


    /*QByteArray m_data;
    bool m_headerParsed;
    int m_contentLength;
    QByteArray m_firstHeaderString;
    QList<QByteArray> m_headers;
    QByteArray m_body;

    QTcpSocket *m_socket;
    QNetworkAccessManager m_manager;*/
    ProxyClientPrivate *d;
};

}

#endif // PROXYCLIENT_H
