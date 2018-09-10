#include "WebSocketClient.h"

#include "Log.h"
#include "check.h"
#include "utils.h"
#include "SlotWrapper.h"
#include "Paths.h"
#include "duration.h"

#include <QTimer>

#include <thread>

WebSocketClient::WebSocketClient(const QString &url, QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QAbstractSocket::SocketState>();
    qRegisterMetaType<std::vector<QString>>();

    m_url = url;

    CHECK(QObject::connect(&thread1,SIGNAL(started()),this,SLOT(onStarted())), "not connect started");

    CHECK(connect(this, SIGNAL(sendMessage(QString)), this, SLOT(onSendMessage(QString))), "not connect sendMessage");
    CHECK(connect(this, SIGNAL(sendMessages(const std::vector<QString> &)), this, SLOT(onSendMessages(const std::vector<QString> &))), "not connect sendMessage");
    CHECK(connect(this, SIGNAL(setHelloString(QString)), this, SLOT(onSetHelloString(QString))), "not connect setHelloString");
    CHECK(connect(this, SIGNAL(setHelloString(const std::vector<QString> &)), this, SLOT(onSetHelloString(const std::vector<QString> &))), "not connect setHelloString");
    CHECK(connect(this, SIGNAL(addHelloString(QString)), this, SLOT(onAddHelloString(QString))), "not connect setHelloString");

    CHECK(connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected), "not connect connected");
    CHECK(connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived), "not connect textMessageReceived");
    CHECK(connect(&m_webSocket, &QWebSocket::disconnected, [this]{
        LOG << "Wss client disconnected";
        m_webSocket.close();
        if (!isStopped) {
            QTimer::singleShot(milliseconds(10s).count(), this, SLOT(onStarted()));
        }
        isConnected = false;
    }), "not connect disconnected");
    CHECK(connect(&thread1, &QThread::finished, [this]{
        LOG << "Wss client finished";
        m_webSocket.close();
    }), "not connect finished");

    moveToThread(&thread1);
    m_webSocket.moveToThread(&thread1);

    LOG << "Wss client started. Url " << m_url.toString();
}

void WebSocketClient::onStarted() {
BEGIN_SLOT_WRAPPER
    m_webSocket.open(m_url);
    LOG << "Wss client onStarted. Url " << m_url.toString();
END_SLOT_WRAPPER
}

WebSocketClient::~WebSocketClient() {
    isStopped = true;
    thread1.quit();
    if (!thread1.wait(3000)) {
        thread1.terminate();
        thread1.wait();
    }
}

void WebSocketClient::start() {
    thread1.start();
}

void WebSocketClient::onConnected() {
BEGIN_SLOT_WRAPPER
    LOG << "Wss client connected";
    isConnected = true;
    for (const QString &helloString: helloStrings) {
        LOG << "Wss Set hello message " << helloString;
        m_webSocket.sendTextMessage(helloString);
    }

    sendMessagesInternal();
END_SLOT_WRAPPER
}

void WebSocketClient::sendMessagesInternal() {
    if (isConnected) {
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

void WebSocketClient::onSetHelloString(QString message) {
BEGIN_SLOT_WRAPPER
    helloStrings.resize(0);
    helloStrings.emplace_back(message);
END_SLOT_WRAPPER
}

void WebSocketClient::onSetHelloString(const std::vector<QString> &messages) {
BEGIN_SLOT_WRAPPER
    helloStrings.assign(messages.begin(), messages.end());
END_SLOT_WRAPPER
}

void WebSocketClient::onAddHelloString(QString message) {
BEGIN_SLOT_WRAPPER
    helloStrings.emplace_back(message);
END_SLOT_WRAPPER
}

void WebSocketClient::onTextMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    LOG << "Wss received " << message;
    emit messageReceived(message);
END_SLOT_WRAPPER
}
