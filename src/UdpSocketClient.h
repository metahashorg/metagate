#ifndef UDPSOCKETCLIENT_H
#define UDPSOCKETCLIENT_H

#include <QObject>
#include <QThread>
#include <QUdpSocket>

#include <functional>

struct TypedException;

class UdpSocketClient : public QObject {
    Q_OBJECT
public:

    using UdpSocketCallback = std::function<void(const std::vector<char> &response)>;

    using ReturnCallback = std::function<void()>;

public:

    explicit UdpSocketClient(QObject *parent = nullptr);

    ~UdpSocketClient();

    void mvToThread(QThread *thread);

    void sendRequest(const QHostAddress &address, int port, const std::vector<char> &request, const UdpSocketCallback &responseCallback);

signals:

    void callbackCall(UdpSocketClient::ReturnCallback callback);

private slots:

    void onReadyRead();

private:

    QUdpSocket socket;

    UdpSocketCallback currentCallback;

    bool isCurrentRequest = false;

};

#endif // UDPSOCKETCLIENT_H
