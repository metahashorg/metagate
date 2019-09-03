#ifndef INIT_PROXY_CLIENT_H
#define INIT_PROXY_CLIENT_H

#include "../InitInterface.h"

#include <QObject>

#include <memory>
#include <future>
#include <functional>

#include "../SharedFuture.h"

struct TypedException;

class MainWindow;

namespace proxy_client {
class ProxyClient;
class ProxyClientJavascript;
}

namespace initializer {

class InitializerJavascript;

class InitProxyClient: public InitInterface {
    Q_OBJECT
public:

    using Return = std::pair<proxy_client::ProxyClient*, proxy_client::ProxyClientJavascript*>;

    using Callback = std::function<void()>;

public:

    InitProxyClient(QThread *mainThread, Initializer &manager);

    ~InitProxyClient() override;

    void completeImpl() override;

    Return initialize(
        SharedFuture<MainWindow> mainWindow
    );

    static int countEvents() {
        return 1;
    }

    static int countCriticalEvents() {
        return 1;
    }

    static QString stateName();

private:

    void sendInitSuccess(const TypedException &exception);

signals:

    void callbackCall(const InitProxyClient::Callback &callback);

private slots:

    void onCallbackCall(const InitProxyClient::Callback &callback);

private:

    std::unique_ptr<proxy_client::ProxyClient> managerProxy;
    std::unique_ptr<proxy_client::ProxyClientJavascript> javascript;

};

}

#endif // INIT_PROXY_CLIENT_H
