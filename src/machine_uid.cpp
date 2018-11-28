#include "machine_uid.h"

#include "check.h"

std::string getMachineUidInternal();

static bool isInitialized = false;
static std::string savedUid;

void initializeMachineUid() {
    savedUid = getMachineUidInternal();
    isInitialized = true;
}

std::string getMachineUid() {
    CHECK(isInitialized, "Not initialized");
    return savedUid;
}
