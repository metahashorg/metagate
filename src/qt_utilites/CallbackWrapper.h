#ifndef CALLBACKWRAPPER_H
#define CALLBACKWRAPPER_H

#include <functional>
#include <memory>
#include <mutex>

struct TypedException;

namespace callbackCall {

class Called {
public:

    ~Called();

    void setCalledFunc();

    void setCalledError();

    bool isCalled() const noexcept;

private:

    bool calledFunc = false;
    bool calledError = false;

    mutable std::mutex mut;
};

using SignalFunc = std::function<void(const std::function<void()> &callback)>;

using ErrorCallback = std::function<void(const TypedException &exception)>;

void wrapErrorImpl(const std::function<void()> &callback, const ErrorCallback &errorCallback);

bool isSetExceptionImpl(const TypedException &exception);

void emitErrorFuncImpl(const SignalFunc &signalFunc, const std::function<void(const TypedException &exception)> &errorCallback, const TypedException &e);

void emitCallbackFuncImpl(const SignalFunc &signalFunc, const std::function<void()> &callback);

}

template<typename Callback>
class CallbackWrapper {
public:

    using SignalFunc = callbackCall::SignalFunc;

    using ErrorCallback = callbackCall::ErrorCallback;

private:

    static std::function<Callback> makeWrapCallback(bool isWrapError, const std::function<Callback> &callback, const ErrorCallback &errorCallback) {
        if (isWrapError) {
            return [callback, errorCallback](auto&& ...args) {
                callbackCall::wrapErrorImpl(std::bind(callback, std::forward<decltype(args)>(args)...), errorCallback);
            };
        } else {
            return callback;
        }
    }

    template<typename OtherCallback>
    static auto makeErrorFunc(const CallbackWrapper<OtherCallback> &errorCallback) {
        return std::bind(&CallbackWrapper<OtherCallback>::emitException, errorCallback, std::placeholders::_1);
    }

public:

    CallbackWrapper() = default;

    CallbackWrapper(const std::function<Callback> &callback, const ErrorCallback &errorCallback, const SignalFunc &signal, bool isWrapError=true)
        : callback(makeWrapCallback(isWrapError, callback, errorCallback))
        , errorCallback(errorCallback)
        , signal(signal)
    {}

    template<typename OtherCallback>
    CallbackWrapper(const std::function<Callback> &callback, const CallbackWrapper<OtherCallback> &errorCallback, const SignalFunc &signal, bool isWrapError=true)
        : callback(makeWrapCallback(isWrapError, callback, makeErrorFunc(errorCallback)))
        , errorCallback(makeErrorFunc(errorCallback))
        , signal(signal)
    {}

    template<typename ...Args>
    void emitCallback(Args&& ...args) const {
        called->setCalledFunc();
        callbackCall::emitCallbackFuncImpl(signal, std::bind(callback, std::forward<Args>(args)...));
    }

    void emitException(const TypedException &exception) const {
        called->setCalledError();
        callbackCall::emitErrorFuncImpl(signal, errorCallback, exception);
    }

    template<typename ...Args>
    void emitFunc(const TypedException &exception, Args&& ...args) const {
        if (callbackCall::isSetExceptionImpl(exception)) {
            emitException(exception);
        } else {
            emitCallback(std::forward<Args>(args)...);
        }
    }

    template<typename ...Args>
    void operator() (const TypedException &exception, Args&& ...args) const {
        emitFunc(exception, std::forward<Args>(args)...);
    }

private:

    std::function<Callback> callback;

    ErrorCallback errorCallback;

    SignalFunc signal;

    std::shared_ptr<callbackCall::Called> called = std::make_shared<callbackCall::Called>();

};

#endif // CALLBACKWRAPPER_H
