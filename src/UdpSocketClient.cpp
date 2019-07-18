#include "UdpSocketClient.h"

#include <QNetworkDatagram>

#include "Log.h"
#include "check.h"
#include "duration.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

UdpSocketClient::UdpSocketClient(QObject *parent)
    : QObject(parent)
{
    Q_CONNECT(&socket, &QUdpSocket::readyRead, this, &UdpSocketClient::onReadyRead);
    Q_CONNECT(&socket, QOverload<QAbstractSocket::SocketError>::of(&QUdpSocket::error), this, &UdpSocketClient::onSocketError);

    Q_CONNECT(&timer, &QTimer::timeout, this, &UdpSocketClient::onTimerEvent);

    timer.setInterval(milliseconds(1s).count());
}

UdpSocketClient::~UdpSocketClient()
{
    //timer.moveToThread(QThread::currentThread());
}

void UdpSocketClient::mvToThread(QThread *thread) {
    this->moveToThread(thread);
    socket.moveToThread(thread);
    CHECK(!isTimerStarted, "Timer already started");
    Q_CONNECT(thread, &QThread::finished, &timer, &QTimer::stop);
    timer.moveToThread(thread);
    thread1 = thread;
}

void UdpSocketClient::startTm() {
    CHECK(!isTimerStarted, "Timer already started");
    if (thread1 != nullptr) {
        Q_CONNECT(thread1, &QThread::started, &timer, QOverload<>::of(&QTimer::start));
    } else {
        timer.start();
    }
    isTimerStarted = true;
}

void UdpSocketClient::onTimerEvent() {
BEGIN_SLOT_WRAPPER
    if (isCurrentRequest) {
        const time_point now = ::now();
        if (now - beginTime >= timeout) {
            socket.abort();
            processResponse(std::vector<char>(), SocketException(1000, "Timeout"));
        }
    }
END_SLOT_WRAPPER
}

void UdpSocketClient::sendRequest(const QHostAddress &address, int port, const std::vector<char> &request, const UdpSocketCallback &responseCallback, milliseconds timeout) {
    CHECK(isTimerStarted, "Timer not started");
    CHECK(!isCurrentRequest, "Request already running");

    beginTime = ::now();
    this->timeout = timeout;

    currentCallback = responseCallback;
    isCurrentRequest = true;
    const auto result = socket.writeDatagram(request.data(), request.size(), address, port);
    if (result == -1) {
        isCurrentRequest = false;
        throwErr("Write udp request error");
    }
}

void UdpSocketClient::closeSock() {
    socket.abort();
}

void UdpSocketClient::processResponse(const std::vector<char> &response, const SocketException &exception) {
    CHECK(isCurrentRequest, "callback not set");
    const UdpSocketCallback copyCallback = currentCallback; // Копируем
    isCurrentRequest = false;
    currentCallback = nullptr;
    emit callbackCall(std::bind(copyCallback, response, exception));
}

void UdpSocketClient::onReadyRead() {
BEGIN_SLOT_WRAPPER
    std::vector<char> response;
    while (socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = socket.receiveDatagram();
        const auto data = datagram.data();
        response.insert(response.end(), data.begin(), data.end());
    }

    processResponse(response, SocketException());
END_SLOT_WRAPPER
}

void UdpSocketClient::onSocketError(QAbstractSocket::SocketError socketError) {
BEGIN_SLOT_WRAPPER
    processResponse(std::vector<char>(), SocketException(socketError, socket.errorString().toStdString()));
END_SLOT_WRAPPER
}
