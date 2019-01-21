#ifndef PROXYJAVASCRIPT_H
#define PROXYJAVASCRIPT_H

#include <QObject>

#include "Proxy.h"
struct TypedException;


namespace proxy
{

class ProxyJavascript : public QObject
{
    Q_OBJECT

public:
    using Callback = std::function<void()>;

public:
    explicit ProxyJavascript(QObject *parent = nullptr);

    Proxy *proxyhManager() const {return m_proxyManager; }

    void setProxyManager(Proxy &proxy)
    {
        m_proxyManager = &proxy;
    }

public slots:
    Q_INVOKABLE void proxyStart();
    Q_INVOKABLE void proxyStop();
    Q_INVOKABLE void getProxyStatus();
    Q_INVOKABLE void getPort();
    Q_INVOKABLE void setPort(quint16 port);
    Q_INVOKABLE void getRoutersList();
    Q_INVOKABLE void discoverRouters();
    Q_INVOKABLE void addPortMapping(const QString &udn);
    Q_INVOKABLE void deletePortMapping();

    Q_INVOKABLE void proxyAutoStartResend();

signals:
    void sendServerStatusResponseSig(const Proxy::ProxyStatus &status, const TypedException &error);

    void sendServerPortResponseSig(quint16 port, const TypedException &error);

    void sendGetRoutersResponseSig(const std::vector<Proxy::Router> &routers, const TypedException &error);

    void sendAutoStartExecutedResponseSig(const TypedException &error);

    void sendAutoStartProxyResponseSig(const Proxy::ProxyResult &res, const TypedException &error);

    void sendAutoStartRouterResponseSig(const Proxy::ProxyResult &res, const TypedException &error);

    void sendAutoStartTestResponseSig(const Proxy::ProxyResult &res, const TypedException &error);

    void sendAutoStartCompleteResponseSig(const TypedException &error);

    void sendAutoStartIsActiveResponseSig(bool active, const TypedException &error);

    void sendConnectedPeersResponseSig(int num, const TypedException &error);

public slots:
    void onSendServerStatusResponseSig(const Proxy::ProxyStatus &status, const TypedException &error);

    void onSendServerPortResponseSig(quint16 port, const TypedException &error);

    void onSendGetRoutersResponseSig(const std::vector<Proxy::Router> &routers, const TypedException &error);

    void onSendAutoStartExecutedResponseSig(const TypedException &error);

    void onSendAutoStartProxyResponseSig(const Proxy::ProxyResult &res, const TypedException &error);

    void onSendAutoStartRouterResponseSig(const Proxy::ProxyResult &res, const TypedException &error);

    void onSendAutoStartTestResponseSig(const Proxy::ProxyResult &res, const TypedException &error);

    void onSendAutoStartCompleteResponseSig(const TypedException &error);

    void onSendAutoStartIsActiveResponseSig(bool active, const TypedException &error);

    void onSendConnectedPeersResponseSig(int num, const TypedException &error);

    void onCallbackCall(const ProxyJavascript::Callback &callback);

signals:
    void callbackCall(const ProxyJavascript::Callback &callback);

    void jsRunSig(QString jsString);

private:
    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

    void runJs(const QString &script);

private:
    Proxy *m_proxyManager;
};

}

#endif // PROXYJAVASCRIPT_H
