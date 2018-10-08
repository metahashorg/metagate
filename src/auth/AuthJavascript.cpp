#include "AuthJavascript.h"

#include "check.h"
#include "SlotWrapper.h"
#include "makeJsFunc.h"



namespace auth
{

AuthJavascript::AuthJavascript(QObject *parent) : QObject(parent)
{

}

void AuthJavascript::login(QString username, QString password)
{

}

template<typename... Args>
void AuthJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

void AuthJavascript::runJs(const QString &script)
{
    LOG << "Javascript " << script;
    emit jsRunSig(script);
}

}
