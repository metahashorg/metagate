#include "QRegister.h"

#include "check.h"

#include <map>
#include <mutex>

static std::map<int, std::string> registered;
static std::mutex mut;

void addToSet(const std::string &name, const std::string &tag, int id, bool isControl) {
    std::lock_guard<std::mutex> lock(mut);
    if (isControl) {
        CHECK(registered.find(id) == registered.end() || registered.at(id) == tag, "Object " + name + " already registered");
    }
    registered[id] = tag;
}
