#include "WebSocketClient.h"

#include "Log.h"
#include "check.h"
#include "utils.h"
#include "SlotWrapper.h"
#include "Paths.h"

#include "uploader.h"

#include <thread>

const QString WEB_SOCKET_SERVER_FILE = "web_socket.txt";

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QAbstractSocket::SocketState>();

    const QString pathToWebSServer = makePath(getSettingsPath(), WEB_SOCKET_SERVER_FILE);
    const std::string &fileData = readFile(pathToWebSServer);
    m_url = QString::fromStdString(fileData).trimmed();

    CHECK(QObject::connect(&thread1,SIGNAL(started()),this,SLOT(onStarted())), "not connect started");

    CHECK(connect(this, SIGNAL(sendMessage(QString)), this, SLOT(onSendMessage(QString))), "not connect sendMessage");
    CHECK(connect(this, SIGNAL(setHelloString(QString)), this, SLOT(onSetHelloString(QString))), "not connect setHelloString");

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
    if (!helloString.isNull() && !helloString.isEmpty()) {
        LOG << "Wss Set hello message " << helloString;
        m_webSocket.sendTextMessage(helloString);
    }
    emit sendMessage("");
END_SLOT_WRAPPER
}

void WebSocketClient::onSendMessage(QString message) {
BEGIN_SLOT_WRAPPER
    if (!isConnected) {
        if (!message.isNull() && !message.isEmpty()) {
            messageQueue.emplace_back(message);
        }
    } else {
        LOG << "Wss client send message " << message;
        for (const QString &m: messageQueue) {
            m_webSocket.sendTextMessage(m);
        }
        messageQueue.clear();
        if (!message.isNull() && !message.isEmpty()) {
            m_webSocket.sendTextMessage(message);
        }
    }
END_SLOT_WRAPPER
}

void WebSocketClient::onSetHelloString(QString message) {
BEGIN_SLOT_WRAPPER
    helloString = message;
END_SLOT_WRAPPER
}

void WebSocketClient::onTextMessageReceived(QString message) {
BEGIN_SLOT_WRAPPER
    LOG << "Wss received " << message;
END_SLOT_WRAPPER
}
