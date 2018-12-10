#ifndef PROXYJAVASCRIPT_H
#define PROXYJAVASCRIPT_H

#include <QObject>

struct TypedException;

namespace proxy
{

class Proxy;

class ProxyJavascript : public QObject
{
    Q_OBJECT
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
    Q_INVOKABLE void getPort();
    Q_INVOKABLE void setPort(quint16 port);

signals:
    void sendServerStatusResponseSig(bool connected, const TypedException &error);

    void sendServerPortResponseSig(quint16 port, const TypedException &error);

public slots:
    void onSendServerStatusResponseSig(bool connected, const TypedException &error);

    void onSendServerPortResponseSig(quint16 port, const TypedException &error);

signals:
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
