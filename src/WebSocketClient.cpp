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

    CHECK(QObject::connect(&thread1,SIGNAL(started()),this,SLOT(onStarted())), "not connect");

    moveToThread(&thread1);
    m_webSocket.moveToThread(&thread1);

    LOG << "Wss client started. Thread " << std::this_thread::get_id() << ". Url " << m_url.toString();
}

void WebSocketClient::onStarted() {
    CHECK(connect(&m_webSocket, &QWebSocket::connected, this, &WebSocketClient::onConnected), "not connect");
    CHECK(connect(&m_webSocket, &QWebSocket::disconnected, [this]{
        LOG << "Wss client disconnected. Thread " << std::this_thread::get_id();
        CHECK(disconnect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived), "not connect");
        CHECK(disconnect(this, SIGNAL(sendMessage(QString)), this, SLOT(onSendMessage(QString))), "not connect");
        m_webSocket.close();
        if (!isStopped) {
            m_webSocket.open(m_url);
        }
    }), "not connect");
    CHECK(connect(&thread1, &QThread::finished, [this]{
        m_webSocket.close();
    }), "not connect");
    m_webSocket.open(m_url);

    LOG << "Wss client started2. Thread " << std::this_thread::get_id() << ". Url " << m_url.toString();
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
    LOG << "Wss client connected. Thread " << std::this_thread::get_id();
    CHECK(connect(&m_webSocket, &QWebSocket::textMessageReceived, this, &WebSocketClient::onTextMessageReceived), "not connect");

    CHECK(connect(this, SIGNAL(sendMessage(QString)), this, SLOT(onSendMessage(QString))), "not connect");
}

void WebSocketClient::onSendMessage(QString message) {
    LOG << "Wss client send message. Thread " << std::this_thread::get_id() << " " << message;
    m_webSocket.sendTextMessage(message);
}

void WebSocketClient::onTextMessageReceived(QString message) {
    LOG << "Wss received. Thread " << std::this_thread::get_id() << " " << message;
}
