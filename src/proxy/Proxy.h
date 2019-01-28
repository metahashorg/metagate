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

    enum State {
        No,
        AutoExecuted,
        AutoProxyStarted,
        AutoUPNPDone,
        AutoComplete,
        AutoError
    };

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

    struct ProxyStatus {
        bool started;
        bool autoActive;
        bool portMapped;
        ProxyStatus(bool started, bool autoActive, bool portMapped)
            : started(started)
            , autoActive(autoActive)
            , portMapped(portMapped)
        {

        }
    };

    struct ProxyResult {
        bool ok;
        QString error;
        ProxyResult(bool ok, const QString &error)
            : ok(ok)
            , error(error)
        {
        }
        ProxyResult(): ok(true)
          , error("")
      {
      }
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
    using ProxyCallback = std::function<void(const ProxyResult &res, const TypedException &exception)>;
    using DiscoverCallback = std::function<void(bool res, const TypedException &exception)>;
    using PortMappingCallback = std::function<void(const PortMappingResult &res, const TypedException &exception)>;

public:

    static std::string moduleName() {
        return "proxy";
    }

public:
    explicit Proxy(ProxyJavascript &javascriptWrapper, QObject *parent = nullptr);
    ~Proxy();

    bool isAutoStart() const;

public slots:
    void startAutoProxy();

    void proxyTested(bool res, const QString &error);

signals:
    void startAutoExecued();

    void startAutoProxyResult(const TypedException &r);

    void startAutoUPnPResult(const TypedException &r);

    void startAutoReadyToTest(quint16 port);

    void startAutoComplete1(bool res);

    void stopProxyExecuted();

signals:
    void proxyStart(const ProxyCallback &callback);

    void proxyStop(const ProxyCallback &callback);

    void geProxyStatus();

    void getPort();

    void setPort(quint16 port);

    void getRouters();

    void discoverRouters(const DiscoverCallback &callback);

    void addPortMapping(const QString &udn, const PortMappingCallback &callback);

    void deletePortMapping(const PortMappingCallback &callback);

    void autoStart(const ProxyCallback &callback);

    void autoStop(const ProxyCallback &callback);


    void autoStartResend();


public slots:

    void onProxyStart(const ProxyCallback &callback);

    void onProxyStop(const ProxyCallback &callback);

    void onGeProxyStatus();

    void onGetPort();

    void onSetPort(quint16 port);

    void onGetRouters();

    void onDiscoverRouters(const DiscoverCallback &callback);

    void onAddPortMapping(const QString &udn, const PortMappingCallback &callback);

    void onDeletePortMapping(const PortMappingCallback &callback);

    void onAutoStart(const ProxyCallback &callback);

    void onAutoStop(const ProxyCallback &callback);

    void onAutoStartResend();

private slots:
    void onRouterDiscovered(UPnPRouter *router);

    void onAutoDiscoveryTimeout();

    void onConnedtedPeersChanged(int peers);

private:
    template<typename Func>
    void runCallback(const Func &callback);
    int findRouter(const QString &udn) const;
    void delPortMapping();

    QThread thread;
    bool m_isAutoStart;

    ProxyJavascript &javascriptWrapper;

    State state;
    ProxyServer *proxyServer;
    UPnPDevices *upnp;
    std::vector<Router> routers;
    int mappedRouterIdx;
    bool proxyStarted = false;

    bool autoProxyRes;
    bool autoRouterRes;
    bool autoTestRes;
    bool autoActive;
    int m_peers;
    bool portMapped;
};

}

#endif // PROXY_H
