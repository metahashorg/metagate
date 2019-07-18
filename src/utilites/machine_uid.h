#ifndef MACHINE_UID_H
#define MACHINE_UID_H

#include <string>

void initializeMachineUid();

std::string getMachineUid();

std::pair<std::string, std::string> findMacAddressFile();

void saveMacAddressesToFile(const std::string &firstAddr, const std::string &secondAddr);

bool isVirtualMachine();

#endif // MACHINE_UID_H
