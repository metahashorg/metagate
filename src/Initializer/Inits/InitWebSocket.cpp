#include "InitWebSocket.h"

#include "WebSocketClient.h"

#include "check.h"
#include "Paths.h"
#include "SlotWrapper.h"

#include <QSettings>

SET_LOG_NAMESPACE("INIT");

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

    registerStateType("init", "websocket initialized", true, true);
    registerStateType("connected", "websocket connected", false, false, 15s, "websocket connected updates");
}

InitWebSocket::~InitWebSocket() = default;

void InitWebSocket::completeImpl() {
    CHECK(webSocket != nullptr, "webSocket not initialized");
}

void InitWebSocket::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

void InitWebSocket::onConnectedSock(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    sendState("connected", false, exception);
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
