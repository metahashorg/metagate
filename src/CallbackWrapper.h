#ifndef CALLBACKWRAPPER_H
#define CALLBACKWRAPPER_H

#include <functional>

#include <QObject>

#include "TypedException.h"

template<typename Callback>
class CallbackWrapper {
public:

    using SignalFunc = std::function<void(const std::function<void()> &callback)>;

    using ErrorCallback = std::function<void(const TypedException &exception)>;

private:

    static Callback makeWrapCallback(bool isWrapError, const Callback &callback, const ErrorCallback &errorCallback) {
        if (isWrapError) {
            return [callback, errorCallback](auto ...args) {
                const TypedException exception = apiVrapper2(std::bind(callback, args...));
                if (exception.isSet()) {
                    errorCallback(exception);
                }
            };
        } else {
            return callback;
        }
    }

public:

    CallbackWrapper() = default;

    CallbackWrapper(const Callback &callback, const ErrorCallback &errorCallback, const SignalFunc &signal, bool isWrapError=true)
        : callback(makeWrapCallback(isWrapError, callback, errorCallback))
        , errorCallback(errorCallback)
        , signal(signal)
    {}

    template<typename ...Args>
    void emitCallback(Args ...args) const {
        emit signal(std::bind(callback, std::forward<Args>(args)...));
    }

    void emitException(const TypedException &exception) const {
        emit signal(std::bind(errorCallback, exception));
    }

    template<typename ...Args>
    void emitFunc(const TypedException &exception, Args ...args) const {
        if (exception.isSet()) {
            emitException(exception);
        } else {
            emitCallback(std::forward<Args>(args)...);
        }
    }

    template<typename ...Args>
    void operator() (const TypedException &exception, Args ...args) const {
        emitFunc(exception, std::forward<Args>(args)...);
    }

private:

    Callback callback;

    ErrorCallback errorCallback;

    SignalFunc signal;

};

#endif // CALLBACKWRAPPER_H
