#include "CallbackWrapper.h"

#include "check.h"
#include "Log.h"
#include "TypedException.h"

#include <QObject>

namespace callbackCall {

Called::~Called() {
    std::unique_lock<std::mutex> lock(mut);
    if (!calledFunc && !calledError) {
        lock.unlock();
        LOG << "Warn. CallbackWrapper not called";
    }
}

void Called::setCalledFunc() {
    std::lock_guard<std::mutex> lock(mut);
    CHECK(!calledFunc, "Callback already called");
    CHECK(!calledError, "Callback already called");
    calledFunc = true;
}

void Called::setCalledError() {
    std::lock_guard<std::mutex> lock(mut);
    calledError = true;
}

void wrapErrorImpl(const std::function<void()> &callback, const ErrorCallback &errorCallback) {
    const TypedException exception = apiVrapper2(callback);
    if (exception.isSet()) {
        errorCallback(exception);
    }
}

bool isSetExceptionImpl(const TypedException &exception) {
    return exception.isSet();
}

void emitErrorFuncImpl(const SignalFunc &signalFunc, const ErrorCallback &errorCallback, const TypedException &e) {
    CHECK(signalFunc, "Callback wrapper not initialized");
    CHECK(errorCallback, "Callback wrapper not initialized");
    emit signalFunc(std::bind(errorCallback, e));
}

void emitCallbackFuncImpl(const SignalFunc &signalFunc, const std::function<void()> &callback) {
    CHECK(signalFunc, "Callback wrapper not initialized");
    emit signalFunc(callback);
}

}
