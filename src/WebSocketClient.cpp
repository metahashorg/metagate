#include "WebSocketClient.h"

#include <QDir>

#include "Log.h"
#include "check.h"
#include "utils.h"

#include "uploader.h"

#include <thread>

const QString WEB_SOCKET_SERVER_FILE = "web_socket.txt";

WebSocketClient::WebSocketClient(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<QAbstractSocket::SocketState>();

    const QString pathToWebSServer = QDir(Uploader::getPagesPath()).filePath(WEB_SOCKET_SERVER_FILE);
    const std::string &fileData = readFile(pathToWebSServer);
    m_url = QString::fromStdString(fileData).trimmed();

    CHECK(QObject::connect(&thread1,SIGNAL(started()),this,SLOT(onStarted())), "not connect started");

    CHECK(connect(this, SIGNAL(sendMessage(QString)), this, SLOT(onSendMessage(QString))), "not connect sendMessage");

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
    m_webSocket.open(m_url);
    LOG << "Wss client onStarted. Url " << m_url.toString();
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
    LOG << "Wss client connected";
    isConnected = true;
}

void WebSocketClient::onSendMessage(QString message) {
    if (!isConnected) {
        messageQueue.emplace_back(message);
    } else {
        LOG << "Wss client send message " << message;
        for (const QString &m: messageQueue) {
            m_webSocket.sendTextMessage(m);
        }
        messageQueue.clear();
        m_webSocket.sendTextMessage(message);
    }
}

void WebSocketClient::onTextMessageReceived(QString message) {
    LOG << "Wss received " << message;
}
