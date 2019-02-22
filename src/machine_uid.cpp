#include "machine_uid.h"

#include "check.h"
#include "Paths.h"
#include "utils.h"

std::string getMachineUidInternal();

bool isVirtualInternal();

static bool isInitialized = false;
static std::string savedUid;
static bool isVirtual = false;

void initializeMachineUid() {
    savedUid = getMachineUidInternal();
    isVirtual = isVirtualInternal();
    isInitialized = true;
}

std::string getMachineUid() {
    CHECK(isInitialized, "Not initialized");
    return savedUid;
}

bool isVirtualMachine() {
    CHECK(isInitialized, "Not initialized");
    return isVirtual;
}

std::pair<std::string, std::string> findMacAddressFile() {
    const QString pathMacAddressFile = getMacFilePath();
    if (!isExistFile(pathMacAddressFile)) {
        return std::make_pair("", "");
    } else {
        const std::string data = readFile(pathMacAddressFile);
        const size_t found = data.find(';');
        if (found == data.npos) {
            return std::make_pair("", "");
        }
        return std::make_pair(data.substr(0, found), data.substr(found + 1));
    }
}

void saveMacAddressesToFile(const std::string &firstAddr, const std::string &secondAddr) {
    const QString pathMacAddressFile = getMacFilePath();
    writeToFile(pathMacAddressFile, firstAddr + ";" + secondAddr, false);
}
