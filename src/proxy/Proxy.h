#ifndef PROXY_H
#define PROXY_H

#include <QObject>
#include <QThread>


struct TypedException;

namespace proxy
{

class ProxyJavascript;
class ProxyServer;
class UPnPDevices;
class UPnPRouter;

class Proxy : public QObject
{
    Q_OBJECT

public:
    struct Router {
        QString friendlyName;
        QString manufacturer;
        QString modelDescription;
        QString modelName;
        QString modelNumber;
        QString serialNumber;
        QString udn;

        bool mapped;
        UPnPRouter *router;
    };

    struct PortMappingResult {
        bool ok;
        quint16 port;
        QString udn;
        QString error;

        PortMappingResult(bool ok, quint16 port, const QString &udn, const QString &error)
            : ok(ok)
            , port(port)
            , udn(udn)
            , error(error)
        {
        }
    };

public:
    using PortMappingCallback = std::function<void(const PortMappingResult &res, const TypedException &exception)>;

public:
    explicit Proxy(ProxyJavascript &javascriptWrapper, QObject *parent = nullptr);
    ~Proxy();

signals:
    void proxyStart();

    void proxyStop();

    void getPort();

    void setPort(quint16 port);

    void getRouters();

    void discoverRouters();

    void addPortMapping(const QString &udn, const PortMappingCallback &callback);

    void deletePortMapping(const PortMappingCallback &callback);


public slots:

    void onProxyStart();

    void onProxyStop();

    void onGetPort();

    void onSetPort(quint16 port);

    void onGetRouters();

    void onDiscoverRouters();

    void onAddPortMapping(const QString &udn, const PortMappingCallback &callback);

    void onDeletePortMapping(const PortMappingCallback &callback);

private slots:
    void onRouterDiscovered(UPnPRouter *router);

private:
    template<typename Func>
    void runCallback(const Func &callback);
    int findRouter(const QString &udn) const;

    QThread thread;

    ProxyJavascript &javascriptWrapper;

    ProxyServer *proxyServer;
    UPnPDevices *upnp;
    std::vector<Router> routers;
    int mappedRouterIdx;
};

}

#endif // PROXY_H
