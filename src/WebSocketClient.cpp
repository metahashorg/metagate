#include "WebSocketClient.h"

#include "Log.h"
#include "check.h"
#include "utils.h"
#include "SlotWrapper.h"
#include "Paths.h"
#include "duration.h"
#include "QRegister.h"

#include <QTimer>

#include <thread>

WebSocketClient::WebSocketClient(const QString &url, QObject *parent)
    : TimerClass(1min, parent)
{
    Q_REG2(QAbstractSocket::SocketState, "QAbstractSocket::SocketState", false);
    Q_REG2(std::vector<QString>, "std::vector<QString>", false);

    m_url = url;
    if (!QSslSocket::supportsSsl()) {
        m_url.setScheme("ws");
    }

    CHECK(connect(this, &WebSocketClient::sendMessage, this, &WebSocketClient::onSendMessage), "not connect sendMessage");
    CHECK(connect(this, &WebSocketClient::sendMessages, this, &WebSocketClient::onSendMessages), "not connect sendMessage");
    CHECK(connect(this, QOverload<QString, QString>::of(&WebSocketClient::setHelloString), this, QOverload<QString, QString>::of(&WebSocketClient::onSetHelloString)), "not connect setHelloString");
    CHECK(connect(this, QOverload<const std::vector<QString>&, QString>::of(&WebSocketClient::setHelloString), this, QOverload<const std::vector<QString>&, QString>::of(&WebSocketClient::onSetHelloString)), "not connect setHelloString");
    CHECK(connect(this, &WebSocketClient::addHelloString, this, &WebSocketClient::onAddHelloString), "not connect setHelloString");

    CHECK(connect(&m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), [this](QAbstractSocket::SocketError error) {
        LOG << "Wss Web socket error " << m_webSocket.errorString() << " " << m_url.toString();
    }), "not connect error");
    CHECK(connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected), "not connect connected");
    CHECK(connect(&m_webSocket, &QWebSocket::pong, this, &WebSocketClient::onPong), "not connect onPong");
    CHECK(connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived), "not connect textMessageReceived");
    CHECK(connect(&m_webSocket, &QWebSocket::disconnected, [this]{
        BEGIN_SLOT_WRAPPER
        LOG << "Wss client disconnected. Url " << m_url.toString();
        m_webSocket.close();
        if (!isStopped) {
            QTimer::singleShot(milliseconds(10s).count(), this, &WebSocketClient::onStarted);
        }
        isConnected = false;
        END_SLOT_WRAPPER
    }), "not connect disconnected");

    prevPongTime = ::now();

    moveToThread(TimerClass::getThread());
    m_webSocket.moveToThread(TimerClass::getThread());

    LOG << "Wss client started. Url " << m_url.toString();
}

void WebSocketClient::onStarted() {
BEGIN_SLOT_WRAPPER
    startMethod();
END_SLOT_WRAPPER
}

void WebSocketClient::startMethod() {
    m_webSocket.open(m_url);
    LOG << "Wss client onStarted. Url " << m_url.toString();
    prevPongTime = ::now();
}

void WebSocketClient::timerMethod() {
    LOG << "Wss check ping " << m_url.toString();
    const time_point now = ::now();
    if (std::chrono::duration_cast<seconds>(now - prevPongTime) >= 3min) {
        LOG << "Wss close " << m_url.toString();
        m_webSocket.close();
    } else {
        emit m_webSocket.ping();
    }
}

void WebSocketClient::finishMethod() {
    LOG << "Wss client finished " << m_url.toString();
    m_webSocket.close();
}

void WebSocketClient::onPong(quint64 elapsedTime, const QByteArray &payload) {
    LOG << "Wss check pong " << m_url.toString();
    prevPongTime = ::now();
}

WebSocketClient::~WebSocketClient() {
    TimerClass::exit();
}

void WebSocketClient::onConnected() {
BEGIN_SLOT_WRAPPER
    LOG << "Wss client connected " << m_url.toString();
    isConnected = true;
    prevPongTime = ::now();
    for (const auto &pair: helloStrings) {
        for (const QString &helloString: pair.second) {
            LOG << "Wss send hello message " << helloString;
            m_webSocket.sendTextMessage(helloString);
        }
    }

    sendMessagesInternal();
    emit connectedSock(TypedException());
END_SLOT_WRAPPER
}

void WebSocketClient::sendMessagesInternal() {
    if (isConnected.load()) {
        LOG << "Wss client send message " << (!messageQueue.empty() ? messageQueue.back() : "") << ". Count " << messageQueue.size();
        for (const QString &m: messageQueue) {
            m_webSocket.sendTextMessage(m);
        }
        messageQueue.clear();
    }
}

void WebSocketClient::onSendMessage(QString message) {
BEGIN_SLOT_WRAPPER
    if (!message.isNull() && !message.isEmpty()) {
        messageQueue.emplace_back(message);
    }

    sendMessagesInternal();
END_SLOT_WRAPPER
}

void WebSocketClient::onSendMessages(const std::vector<QString> &messages) {
BEGIN_SLOT_WRAPPER
    messageQueue.insert(messageQueue.end(), messages.begin(), messages.end());

    sendMessagesInternal();
END_SLOT_WRAPPER
}

void WebSocketClient::onSetHelloString(QString message, QString tag) {
BEGIN_SLOT_WRAPPER
    helloStrings[tag].clear();
    helloStrings[tag].emplace_back(message);
END_SLOT_WRAPPER
}

void WebSocketClient::onSetHelloString(const std::vector<QString> &messages, QString tag) {
BEGIN_SLOT_WRAPPER
    helloStrings[tag].assign(messages.begin(), messages.end());
END_SLOT_WRAPPER
}

void WebSocketClient::onAddHelloString(QString message, QString tag) {
BEGIN_SLOT_WRAPPER
    helloStrings[tag].emplace_back(message);
END_SLOT_WRAPPER
}

void WebSocketClient::onTextMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    LOG << "Wss received part: " << message.left(2000);
    emit messageReceived(message);
END_SLOT_WRAPPER
}
