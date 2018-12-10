#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include <QThread>

namespace proxy
{

class ProxyJavascript;
class ProxyServer;
class UPnPDevices;

class Proxy : public QObject
{
    Q_OBJECT
public:
    explicit Proxy(ProxyJavascript &javascriptWrapper, QObject *parent = nullptr);
    ~Proxy();

signals:
    void proxyStart();

    void proxyStop();

    void getPort();

    void setPort(quint16 port);


public slots:

    void onProxyStart();

    void onProxyStop();

    void onGetPort();

    void onSetPort(quint16 port);


private:
    QThread thread;

    ProxyJavascript &javascriptWrapper;

    ProxyServer *proxyServer;
    UPnPDevices *upnp;

};

}

#endif // PROXY_H
