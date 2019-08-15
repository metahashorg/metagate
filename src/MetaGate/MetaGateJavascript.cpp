#include "MetaGateJavascript.h"

#include "Log.h"
#include "check.h"

#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"
#include "qt_utilites/WrapperJavascriptImpl.h"

SET_LOG_NAMESPACE("MG");

namespace metagate {

MetaGateJavascript::MetaGateJavascript(MetaGate &metagate)
    : WrapperJavascript(false, LOG_FILE)
    , metagate(metagate)
{

}

} // namespace metagate
