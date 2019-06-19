#include "WrapperJavascript.h"

#include "check.h"
#include "QRegister.h"
#include "SlotWrapper.h"
#include "Log.h"

#include "makeJsFunc.h"

using namespace std::placeholders;

WrapperJavascript::WrapperJavascript(bool printJs, const std::string &cppFileName)
    : printJs(printJs)
    , cppFileName(cppFileName)
{
    CHECK(connect(this, &WrapperJavascript::callbackCall, this, &WrapperJavascript::onCallbackCall), "not connect onCallbackCall");

    Q_REG(WrapperJavascript::Callback, "WrapperJavascript::Callback");

    signalFunc = std::bind(&WrapperJavascript::callbackCall, this, _1);
}

void WrapperJavascript::runJs(const QString &script) {
    if (printJs) {
        LOG2(cppFileName) << "Javascript " << script;
    }
    emit jsRunSig(script);
}

void WrapperJavascript::onCallbackCall(const std::function<void()> &callback) {
BEGIN_SLOT_WRAPPER
    callback();
END_SLOT_WRAPPER
}

void WrapperJavascript::wrapOperation(const std::function<void()> &f, const std::function<void(const TypedException &e)> &errorFunc) {
    const TypedException exception = apiVrapper2(f);

    if (exception.isSet()) {
        errorFunc(exception);
    }
}
