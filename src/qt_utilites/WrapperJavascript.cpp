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
{}

WrapperJavascript::~WrapperJavascript() = default;

void WrapperJavascript::runJs(const QString &script) {
    if (printJs) {
        LOG2(cppFileName) << "Javascript " << script;
    }
    emit jsRunSig(script);
}

void WrapperJavascript::wrapOperation(const std::function<void()> &f, const std::function<void(const TypedException &e)> &errorFunc) {
    const TypedException exception = apiVrapper2(f);

    if (exception.isSet()) {
        errorFunc(exception);
    }
}
