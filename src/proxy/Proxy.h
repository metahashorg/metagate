#ifndef PROXY_H
#define PROXY_H

#include <QObject>

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

signals:
    void proxyStart();

    void proxyStop();

public slots:

    void onProxyStart();

    void onProxyStop();


private:
    ProxyJavascript &javascriptWrapper;

    ProxyServer *proxyServer;
    UPnPDevices *upnp;

};

}

#endif // PROXY_H
