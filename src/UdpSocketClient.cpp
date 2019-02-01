#include "UdpSocketClient.h"

#include <QNetworkDatagram>

#include "Log.h"
#include "check.h"
#include "SlotWrapper.h"
#include "QRegister.h"

UdpSocketClient::UdpSocketClient(QObject *parent)
    : QObject(parent)
{
    CHECK(connect(&socket, &QUdpSocket::readyRead, this, &UdpSocketClient::onReadyRead), "not connect onReadyRead");
}

UdpSocketClient::~UdpSocketClient() = default;

void UdpSocketClient::mvToThread(QThread *thread) {
    this->moveToThread(thread);
    socket.moveToThread(thread);
}

void UdpSocketClient::sendRequest(const QHostAddress &address, int port, const std::vector<char> &request, const UdpSocketCallback &responseCallback) {
    CHECK(!isCurrentRequest, "Request already running");
    currentCallback = responseCallback;
    isCurrentRequest = true;
    const auto result = socket.writeDatagram(request.data(), request.size(), address, port);
    if (result == -1) {
        isCurrentRequest = false;
        throwErr("Write udp request error");
    }
}

void UdpSocketClient::onReadyRead() {
BEGIN_SLOT_WRAPPER
    CHECK(isCurrentRequest, "callback not set");
    const UdpSocketCallback copyCallback = currentCallback;
    isCurrentRequest = false;

    std::vector<char> response;
    while (socket.hasPendingDatagrams()) {
        const QNetworkDatagram datagram = socket.receiveDatagram();
        response.insert(response.end(), datagram.data().begin(), datagram.data().end());
    }

    emit callbackCall(std::bind(copyCallback, response));
END_SLOT_WRAPPER
}
