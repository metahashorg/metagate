#include "Module.h"

#include "check.h"
#include "Log.h"

#include <unordered_map>
#include <algorithm>
#include <mutex>
#include <thread>

#include <QTimer>

static std::mutex mut;
static std::unordered_map<std::string, StatusModule> modules;

static bool isModulesInit = false;

static QTimer timer;

static bool isRunningApplication = false;

static std::thread::id threadId;

void initModules() {
    CHECK(!isModulesInit, "Already initialized");

    timer.setInterval(0);
    timer.setSingleShot(true);
    CHECK(QObject::connect(&timer, &QTimer::timeout, [](){
        isRunningApplication = true;
    }), "not connect");
    timer.start();

    threadId = std::this_thread::get_id();

    isModulesInit = true;
}

void addModule(const std::string &moduleName) {
    CHECK(isModulesInit, "Modules not init");
    CHECK(!isRunningApplication, "Application already running");
    CHECK(std::this_thread::get_id() == threadId, "thread not same");

    std::lock_guard<std::mutex> lock(mut);
    modules.emplace(moduleName, StatusModule::wait);
}

void changeStatus(const std::string &moduleName, StatusModule status) {
    CHECK(isModulesInit, "Modules not init");
    std::lock_guard<std::mutex> lock(mut);
    CHECK(modules.find(moduleName) != modules.end(), "Module not added");
    modules[moduleName] = status;
}

std::vector<std::pair<std::string, StatusModule>> getStatusModules() {
    CHECK(isModulesInit, "Modules not init");
    std::vector<std::pair<std::string, StatusModule>> result;
    result.reserve(modules.size());
    std::lock_guard<std::mutex> lock(mut);
    std::transform(modules.begin(), modules.end(), std::back_inserter(result), [](const auto &pair) {
        return std::make_pair(pair.first, pair.second);
    });
    return result;
}
