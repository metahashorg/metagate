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

public:

    CallbackWrapper() = default;

    CallbackWrapper(const Callback &callback, const ErrorCallback &errorCallback, const SignalFunc &signal)
        : callback(callback)
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

private:

    Callback callback;

    ErrorCallback errorCallback;

    SignalFunc signal;

};

#endif // CALLBACKWRAPPER_H
