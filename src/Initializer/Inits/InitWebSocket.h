#ifndef INIT_WEBSOCKET_H
#define INIT_WEBSOCKET_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>

struct TypedException;

class WebSocketClient;

namespace initializer {

class InitializerJavascript;

class InitWebSocket: public InitInterface {
    Q_OBJECT
public:

    using Return = WebSocketClient*;

public:

    InitWebSocket(QThread *mainThread, Initializer &manager);

    ~InitWebSocket() override;

    void complete() override;

    Return initialize();

    static int countEvents() {
        return 2;
    }

signals:

    void connectedSock(const TypedException &exception);

private slots:

    void onConnectedSock(const TypedException &exception);

private:

    void sendInitSuccess(const TypedException &exception);

private:

    std::unique_ptr<WebSocketClient> webSocket;

    bool isConnected = false;

};

}

#endif // INIT_WEBSOCKET_H
