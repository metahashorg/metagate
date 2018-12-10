#include "InitNsLookup.h"

#include "../Initializer.h"

#include <functional>
using namespace std::placeholders;

#include "NsLookup.h"

#include "check.h"
#include "SlotWrapper.h"

namespace initializer {

InitNsLookup::InitNsLookup(QThread *mainThread, Initializer &manager)
    : InitInterface(mainThread, manager, true)
{
    CHECK(connect(this, &InitNsLookup::serversFlushed, this, &InitNsLookup::onServersFlushed), "not connect onServersFlushed");
    setTimerEvent(50s, "nslookup flushed timeout", std::bind(&InitNsLookup::serversFlushed, this, _1));
}

InitNsLookup::~InitNsLookup() = default;

void InitNsLookup::complete() {
    CHECK(nsLookup != nullptr, "nsLookup not initialized");
    CHECK(isFlushed, "Not flushed");
}

void InitNsLookup::sendInitSuccess(const TypedException &exception) {
    sendState(InitState("nslookup", "init", "nslookup initialized", exception));
}

void InitNsLookup::onServersFlushed(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    if (!isFlushed) {
        sendState(InitState("nslookup", "flushed", "nslookup flushed", exception));
        isFlushed = true;
    }
END_SLOT_WRAPPER
}

InitNsLookup::Return InitNsLookup::initialize() {
    const TypedException exception = apiVrapper2([&, this] {
        nsLookup = std::make_unique<NsLookup>();
        CHECK(connect(nsLookup.get(), &NsLookup::serversFlushed, this, &InitNsLookup::serversFlushed), "not connect onServersFlushed");
        nsLookup->start();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return nsLookup.get();
}

}
