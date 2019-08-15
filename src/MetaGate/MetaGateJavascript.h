#ifndef METAGATEJAVASCRIPT_H
#define METAGATEJAVASCRIPT_H

#include "qt_utilites/WrapperJavascript.h"

namespace metagate {

class MetaGate;

class MetaGateJavascript: public WrapperJavascript{
public:
    MetaGateJavascript(MetaGate &metagate);

private:

    MetaGate &metagate;
};

} // namespace metagate

#endif // METAGATEJAVASCRIPT_H
