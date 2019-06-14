#ifndef WRAPPERJAVASCRIPT_H
#define WRAPPERJAVASCRIPT_H

#include <QObject>

#include <functional>

#include "makeJsFunc.h"

struct TypedException;

class WrapperJavascript : public QObject {
    Q_OBJECT
public:

    using Callback = std::function<void()>;

protected:

    template<typename T>
    struct JsTypeReturn {
        using type = T;

        /*
           Default value for error result
        */
        explicit JsTypeReturn(T &&value)
            : value(std::forward<T>(value))
        {}

        /*
           Default value for error result
        */
        explicit JsTypeReturn(const T &value)
            : value(value)
        {}

        T value;
    };

    template<typename T1, typename T2>
    struct FuncPair {
        T1 func;
        T2 error;

        FuncPair(T1 &&func, T2 &&func2)
            : func(std::forward<T1>(func))
            , error(std::forward<T2>(func2))
        {}
    };

    template<typename T1, typename T2>
    constexpr static FuncPair<typename std::decay_t<T1>, typename std::decay_t<T2>> make_func(T1&& x, T2&& y) {
        typedef typename std::decay_t<T1> ds_type1;
        typedef typename std::decay_t<T2> ds_type2;
        typedef WrapperJavascript::FuncPair<ds_type1, ds_type2> pair_type;
        return pair_type(std::forward<T1>(x), std::forward<T2>(y));
    }

public:

    WrapperJavascript(bool printJs);

signals:

    void jsRunSig(QString jsString);

    void callbackCall(const WrapperJavascript::Callback &callback);

private slots:

    void onCallbackCall(const WrapperJavascript::Callback &callback);

protected:

    template<typename ...Args>
    auto makeJavascriptReturnAndErrorFuncs(const QString &jsNameResult, Args&& ...args) {
        return make_func([jsNameResult, this](const TypedException &exception, const typename Args::type& ...args) {
            makeAndRunJsFuncParams(jsNameResult, exception, args...);
        }, [=, this](const TypedException &exception){
            makeAndRunJsFuncParams(jsNameResult, exception, args.value...);
        });
    }

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args) {
        const QString res = makeJsFunc3<false>(function, "", exception, std::forward<Args>(args)...);
        runJs(res);
    }

private:

    void runJs(const QString &script);

protected:

    const bool printJs;

    std::function<void(const std::function<void()> &callback)> signalFunc;
};

#endif // WRAPPERJAVASCRIPT_H
