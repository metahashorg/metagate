#ifndef INIT_WEBSOCKET_H
#define INIT_WEBSOCKET_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>

class TypedException;

class WebSocketClient;

namespace initializer {

class InitializerJavascript;

class InitWebSocket: public QObject, public InitInterface {
    Q_OBJECT
public:

    using Return = std::reference_wrapper<WebSocketClient>;

public:

    InitWebSocket(QThread *mainThread, Initializer &manager);

    ~InitWebSocket() override;

    void complete() override;

    Return initialize();

    static int countEvents() {
        return 2;
    }

private slots:

    void onConnectedSock();

private:

    void sendInitSuccess(const TypedException &exception);

    void sendConnected(const TypedException &exception);

private:

    std::unique_ptr<WebSocketClient> webSocket;

    bool isConnected = false;

};

}

#endif // INIT_WEBSOCKET_H
