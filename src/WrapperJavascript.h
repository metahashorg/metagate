#ifndef WRAPPERJAVASCRIPT_H
#define WRAPPERJAVASCRIPT_H

#include <QObject>

#include <functional>

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

    WrapperJavascript(bool printJs, const std::string &cppFileName);

signals:

    void jsRunSig(QString jsString);

    void callbackCall(const WrapperJavascript::Callback &callback);

private slots:

    void onCallbackCall(const WrapperJavascript::Callback &callback);

protected:

    template<typename ...Args>
    auto makeJavascriptReturnAndErrorFuncs(const QString &jsNameResult, Args&& ...args);

    template<typename... Args>
    void makeAndRunJsFuncParams(const QString &function, const TypedException &exception, Args&& ...args);

protected:

    void wrapOperation(const std::function<void()> &f, const std::function<void(const TypedException &e)> &errorFunc);

private:

    void runJs(const QString &script);

protected:

    const bool printJs;

    const std::string cppFileName;

    std::function<void(const std::function<void()> &callback)> signalFunc;
};

#endif // WRAPPERJAVASCRIPT_H
