#include "InitWebSocket.h"

#include "../Initializer.h"

#include "WebSocketClient.h"

#include <functional>
using namespace std::placeholders;

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

QString InitWebSocket::stateName() {
    return "websocket";
}

InitWebSocket::InitWebSocket(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, true)
{
    CHECK(connect(this, &InitWebSocket::connectedSock, this, &InitWebSocket::onConnectedSock), "not connect onConnectedSock");
    setTimerEvent(15s, "websocket connected updates", std::bind(&InitWebSocket::connectedSock, this, _1));
}

InitWebSocket::~InitWebSocket() = default;

void InitWebSocket::complete() {
    CHECK(webSocket != nullptr, "webSocket not initialized");
    CHECK(isConnected, "webSocket not connected");
}

void InitWebSocket::sendInitSuccess(const TypedException &exception) {
    sendState(InitState(stateName(), "init", "websocket initialized", true, exception));
}

void InitWebSocket::onConnectedSock(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    if (!isConnected) {
        sendState(InitState(stateName(), "connected", "websocket connected", false, exception));
        isConnected = true;
    }
END_SLOT_WRAPPER
}

InitWebSocket::Return InitWebSocket::initialize() {
    const TypedException exception = apiVrapper2([&, this] {
        webSocket = std::make_unique<WebSocketClient>(getUrlToWss());
        CHECK(connect(webSocket.get(), &WebSocketClient::connectedSock, this, &InitWebSocket::connectedSock), "not connect onConnectedSock");
        webSocket->start();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return webSocket.get();
}

}
