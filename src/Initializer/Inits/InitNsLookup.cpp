#include "InitNsLookup.h"

#include "NsLookup/NsLookup.h"

#include "check.h"
#include "qt_utilites/SlotWrapper.h"
#include "qt_utilites/QRegister.h"

SET_LOG_NAMESPACE("INIT");

namespace initializer {

QString InitNsLookup::stateName() {
    return "nslookup";
}

InitNsLookup::InitNsLookup(QThread *mainThread, Initializer &manager)
    : InitInterface(stateName(), mainThread, manager, true)
{
    Q_CONNECT(this, &InitNsLookup::serversFlushed, this, &InitNsLookup::onServersFlushed);

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
        Q_CONNECT(nsLookup.get(), &NsLookup::serversFlushed, this, &InitNsLookup::serversFlushed);
        nsLookup->start();
    });
    sendInitSuccess(exception);
    if (exception.isSet()) {
        throw exception;
    }
    return nsLookup.get();
}

}
