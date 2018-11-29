#ifndef PROXYCLIENT_H
#define PROXYCLIENT_H

#include <QTcpSocket>
#include <QNetworkAccessManager>

class ProxyClient : public QTcpSocket
{
    Q_OBJECT
public:
    explicit ProxyClient(QObject *parent = nullptr);

    virtual bool event(QEvent *e) override;

private slots:
    void onConnected();
    void onError(QAbstractSocket::SocketError socketError);
    void onReadyRead();

private:
    void startNewQuery();
    void parse();
    void parseHeader();
    void sendQuery();

    QByteArray m_data;
    bool m_headerParsed;
    int m_contentLength;
    QByteArray m_firstHeaderString;
    QList<QByteArray> m_headers;
    QByteArray m_body;

    QNetworkAccessManager m_manager;
};

#endif // PROXYCLIENT_H
