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
SET_LOG_NAMESPACE("WSS");

WebSocketClient::WebSocketClient(const QString &url, QObject *parent)
    : TimerClass(1min, parent)
    , m_webSocket(new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this))
{
    Q_REG2(QAbstractSocket::SocketState, "QAbstractSocket::SocketState", false);
    Q_REG2(std::vector<QString>, "std::vector<QString>", false);

    m_url = url;
    if (!QSslSocket::supportsSsl()) {
        m_url.setScheme("ws");
    }

    Q_CONNECT(this, &WebSocketClient::sendMessage, this, &WebSocketClient::onSendMessage);
    Q_CONNECT(this, &WebSocketClient::sendMessages, this, &WebSocketClient::onSendMessages);
    Q_CONNECT(this, (QOverload<QString, QString>::of(&WebSocketClient::setHelloString)), this, (QOverload<QString, QString>::of(&WebSocketClient::onSetHelloString)));
    Q_CONNECT(this, (QOverload<const std::vector<QString>&, QString>::of(&WebSocketClient::setHelloString)), this, (QOverload<const std::vector<QString>&, QString>::of(&WebSocketClient::onSetHelloString)));
    Q_CONNECT(this, &WebSocketClient::addHelloString, this, &WebSocketClient::onAddHelloString);

    Q_CONNECT3(m_webSocket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), [this](QAbstractSocket::SocketError error) {
        LOG << "Wss Web socket error " << m_webSocket->errorString() << " " << m_url.toString();
    });
    Q_CONNECT(m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected);
    Q_CONNECT(m_webSocket, &QWebSocket::pong, this, &WebSocketClient::onPong);
    Q_CONNECT(m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived);
    Q_CONNECT3(m_webSocket, &QWebSocket::disconnected, [this]{
        BEGIN_SLOT_WRAPPER
        LOG << "Wss client disconnected. Url " << m_url.toString();
        m_webSocket->close();
        if (!isStopped) {
            QTimer::singleShot(milliseconds(10s).count(), this, &WebSocketClient::onStarted);
        }
        isConnected = false;
        END_SLOT_WRAPPER
    });

    prevPongTime = ::now();

    moveToThread(TimerClass::getThread());
    m_webSocket->moveToThread(TimerClass::getThread());

    LOG << "Wss client started. Url " << m_url.toString();
}

void WebSocketClient::onStarted() {
BEGIN_SLOT_WRAPPER
    startMethod();
END_SLOT_WRAPPER
}

void WebSocketClient::startMethod() {
    m_webSocket->open(m_url);
    LOG << "Wss client onStarted. Url " << m_url.toString();
    prevPongTime = ::now();
}

void WebSocketClient::timerMethod() {
    LOG << "Wss check ping " << m_url.toString();
    const time_point now = ::now();
    if (std::chrono::duration_cast<seconds>(now - prevPongTime) >= 3min) {
        LOG << "Wss close " << m_url.toString();
        m_webSocket->close();
    } else {
        emit m_webSocket->ping();
    }
}

void WebSocketClient::finishMethod() {
    LOG << "Wss client finished " << m_url.toString();
    m_webSocket->close();
}

void WebSocketClient::onPong(quint64 elapsedTime, const QByteArray &payload) {
    LOG << "Wss check pong " << m_url.toString();
    prevPongTime = ::now();
}

WebSocketClient::~WebSocketClient() {
    TimerClass::exit();
    isStopped = true;
}

void WebSocketClient::onConnected() {
BEGIN_SLOT_WRAPPER
    LOG << "Wss client connected " << m_url.toString();
    isConnected = true;
    prevPongTime = ::now();
    for (const auto &pair: helloStrings) {
        for (const QString &helloString: pair.second) {
            m_webSocket->sendTextMessage(helloString);
        }
    }

    sendMessagesInternal();
    emit connectedSock(TypedException());
END_SLOT_WRAPPER
}

void WebSocketClient::sendMessagesInternal() {
    if (isConnected.load()) {
        for (const QString &m: messageQueue) {
            m_webSocket->sendTextMessage(m);
        }
        messageQueue.clear();
    }
}

void WebSocketClient::onSendMessage(QString message) {
BEGIN_SLOT_WRAPPER
    //LOG << m_url.toString() << " WSS SEND MESSAGE'" << message << "'";
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
    LOG << "Wss received size: " << message.size();
    //LOG << m_url.toString() << " WSS GET: '" << message << "'";
    emit messageReceived(message);
END_SLOT_WRAPPER
}
