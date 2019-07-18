#include "CallbackCallWrapper.h"

#include "check.h"
#include "QRegister.h"
#include "SlotWrapper.h"
#include "Log.h"

using namespace std::placeholders;

CallbackCallWrapper::CallbackCallWrapper(QObject *parent)
    : QObject(parent)
{
    Q_CONNECT(this, &CallbackCallWrapper::callbackCall, this, &CallbackCallWrapper::onCallbackCall);

    Q_REG(CallbackCallWrapper::Callback, "CallbackCallWrapper::Callback");

    signalFunc = std::bind(&CallbackCallWrapper::callbackCall, this, _1);
}

CallbackCallWrapper::~CallbackCallWrapper() = default;

void CallbackCallWrapper::onCallbackCall(const std::function<void()> &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}
