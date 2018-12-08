#include "ProxyJavascript.h"

#include "Proxy.h"

#include "check.h"
#include "SlotWrapper.h"
#include "makeJsFunc.h"

#include <QDebug>

namespace proxy
{

ProxyJavascript::ProxyJavascript(QObject *parent)
    : QObject(parent)
{

}

void ProxyJavascript::proxyStart()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxy not set");

    qDebug() << "!!!";
    LOG << "Proxy start";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->proxyStart();
    });
    //if (exception.isSet()) {
    //    emit sendLoginInfoResponseSig(LoginInfo(), exception);
    //}
END_SLOT_WRAPPER
}

void ProxyJavascript::proxyStop()
{
BEGIN_SLOT_WRAPPER
    CHECK(m_proxyManager, "proxy not set");
    LOG << "Proxy stop";

    const TypedException exception = apiVrapper2([&, this]() {
        emit m_proxyManager->proxyStop();
    });
END_SLOT_WRAPPER
}

template<typename... Args>
void ProxyJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void ProxyJavascript::runJs(const QString &script)
{
    LOG << "Js " << script;
    emit jsRunSig(script);
}

}
