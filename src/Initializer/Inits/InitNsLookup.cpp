#include "InitNsLookup.h"

#include "NsLookup.h"

#include "check.h"
#include "SlotWrapper.h"

namespace initializer {

QString InitNsLookup::stateName() {
    return "nslookup";
}

InitNsLookup::InitNsLookup(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, true)
{
    CHECK(connect(this, &InitNsLookup::serversFlushed, this, &InitNsLookup::onServersFlushed), "not connect onServersFlushed");

    registerStateType("init", "nslookup initialized", true, true);
    registerStateType("flushed", "nslookup flushed", false, false, 50s, "nslookup flushed timeout");
}

InitNsLookup::~InitNsLookup() = default;

void InitNsLookup::completeImpl() {
    CHECK(nsLookup != nullptr, "nsLookup not initialized");
}

void InitNsLookup::sendInitSuccess(const TypedException &exception) {
    sendState("init", false, exception);
}

void InitNsLookup::onServersFlushed(const TypedException &exception) {
BEGIN_SLOT_WRAPPER
    sendState("flushed", false, exception);
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
