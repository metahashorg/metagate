#include "InitWebSocket.h"

#include "../Initializer.h"

#include "WebSocketClient.h"

#include "check.h"
#include "Paths.h"
#include "SlotWrapper.h"

#include <QSettings>

static QString getUrlToWss() {
    QSettings settings(getSettingsPath(), QSettings::IniFormat);
    CHECK(settings.contains("web_socket/meta_online"), "web_socket/meta_online not found setting");
    return settings.value("web_socket/meta_online").toString();
}

namespace initializer {

InitWebSocket::InitWebSocket(QThread *mainThread, Initializer &manager)
    : InitInterface(mainThread, manager)
{
    QTimer::singleShot(milliseconds(15s).count(), [this]{
        onConnectedSock(TypedException(INITIALIZER_TIMEOUT_ERROR, "websocket connected updates"));
    });
}

InitWebSocket::~InitWebSocket() = default;

void InitWebSocket::complete() {
    CHECK(webSocket != nullptr, "webSocket not initialized");
    CHECK(isConnected, "webSocket not connected");
}

void InitWebSocket::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("websocket", "init", "websocket initialized", exception));
}

void InitWebSocket::onConnectedSock(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    if (!isConnected) {
        sendState(InitState("websocket", "connected", "websocket connected", exception));
        isConnected = true;
    }
END_SLOT_WRAPPER
}

InitWebSocket::Return InitWebSocket::initialize() {
    const TypedException exception = apiVrapper2([&, this] {
        webSocket = std::make_unique<WebSocketClient>(getUrlToWss());
        CHECK(connect(webSocket.get(), &WebSocketClient::connectedSock, this, &InitWebSocket::onConnectedSock), "not connect onConnectedSock");
        webSocket->start();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return webSocket.get();
}

}
