#ifndef WRAPPERJAVASCRIPTIMPL_H
#define WRAPPERJAVASCRIPTIMPL_H

#include "WrapperJavascript.h"

#include "makeJsFunc.h"

template<typename ...Args>
auto WrapperJavascript::makeJavascriptReturnAndErrorFuncs(const QString &jsNameResult, Args&& ...args) {
    return make_func([jsNameResult, this](const TypedException &exception, const typename Args::type& ...args) {
        makeAndRunJsFuncParams(jsNameResult, exception, args...);
    }, [=](const TypedException &exception){
        makeAndRunJsFuncParams(jsNameResult, exception, args.value...);
    });
}

template<typename... Args>
void WrapperJavascript::makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
    const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
    runJs(res);
}

#endif // WRAPPERJAVASCRIPTIMPL_H
