#ifndef UDPSOCKETCLIENT_H
#define UDPSOCKETCLIENT_H

#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QTimer>

#include <functional>

#include "duration.h"

struct TypedException;

class UdpSocketClient : public QObject {
    Q_OBJECT
public:

    struct SocketException {

        SocketException() = default;

        SocketException(int code, const std::string &description)
            : description(description)
            , code(code)
        {}

        std::string description;

        int code = 0;

        bool isSet() const {
            return code != 0;
        }

        std::string toString() const {
            return description;
        }

    };

public:

    using UdpSocketCallback = std::function<void(const std::vector<char> &response, const SocketException &exception)>;

    using ReturnCallback = std::function<void()>;

public:

    explicit UdpSocketClient(QObject *parent = nullptr);

    ~UdpSocketClient();

    void mvToThread(QThread *thread);

    void sendRequest(const QHostAddress &address, int port, const std::vector<char> &request, const UdpSocketCallback &responseCallback, milliseconds timeout);

    void startTm();

    void closeSock();

signals:

    void callbackCall(UdpSocketClient::ReturnCallback callback);

private slots:

    void onReadyRead();

    void onTimerEvent();

    void onSocketError(QAbstractSocket::SocketError socketError);

private:

    void processResponse(const std::vector<char> &response, const SocketException &exception);

private:

    QUdpSocket socket;

    QThread *thread1 = nullptr;

    QTimer timer;

    bool isTimerStarted = false;

    UdpSocketCallback currentCallback;

    bool isCurrentRequest = false;

    time_point beginTime;

    milliseconds timeout;

};

#endif // UDPSOCKETCLIENT_H
