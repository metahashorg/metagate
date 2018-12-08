#include "Proxy.h"

#include "ProxyJavascript.h"
#include "ProxyServer.h"
#include "UPnPDevices.h"
#include "UPnPRouter.h"

#include "check.h"
#include "SlotWrapper.h"

namespace proxy
{

Proxy::Proxy(ProxyJavascript &javascriptWrapper, QObject *parent)
    : QObject(parent)
    , javascriptWrapper(javascriptWrapper)
    , proxyServer(new ProxyServer(this))
    , upnp(new UPnPDevices(this))
{
    CHECK(connect(this, &Proxy::proxyStart, this, &Proxy::onProxyStart), "not connect onProxyStart");
    CHECK(connect(this, &Proxy::proxyStop, this, &Proxy::onProxyStop), "not connect onProxyStop");

    javascriptWrapper.setProxyManager(*this);
    // TODO to new thread
    //moveToThread(&thread1);
}

void Proxy::onProxyStart()
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        proxyServer->start();
    });
END_SLOT_WRAPPER
    //emit javascriptWrapper.sendLoginInfoResponseSig(info, exception);
}

void Proxy::onProxyStop()
{
BEGIN_SLOT_WRAPPER
    const TypedException exception = apiVrapper2([&, this] {
        proxyServer->stop();
    });
END_SLOT_WRAPPER
}

}
