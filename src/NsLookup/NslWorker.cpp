#include "NslWorker.h"

#include "duration.h"
#include "check.h"
#include "Log.h"

#include "TaskManager.h"

#include "Workers/FullWorker.h"
#include "Workers/SimpleWorker.h"
#include "Workers/RefreshIpWorker.h"
#include "Workers/RefreshNodeWorker.h"
#include "Workers/FindEmptyNodesWorker.h"
#include "Workers/PrintNodesWorker.h"
#include "Workers/MiddleWorker.h"

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

NslWorker::NslWorker(TaskManager &manager)
    : manager(manager)
{}

NslWorker::~NslWorker() {
    if (!finished) {
        LOG << "WARN: " << "Nsl worker not finished";
    }
}

bool NslWorker::isActual() const {
    return checkIsActual();
}

void NslWorker::runWork(WorkerGuard workerGuard) {
    finished = false;
    runWorkImpl(workerGuard);
}

void NslWorker::addNewTask(const Task &task) {
    manager.addTask(task);
}

void NslWorker::addSpentRecord() {
    const std::string type = getType();
    const std::string subType = getSubType();
    const system_time_point now = ::system_now();

    manager.addSpentTask(TaskRecord(type, subType, now));
}

bool NslWorker::findSpentRecord(TaskRecord &result) const {
    const std::string type = getType();
    const std::string subType = getSubType();

    return manager.findSpentTask(type, subType, result);
}

void NslWorker::finishWork(WorkerGuard workerGuard) {
    CHECK(workerGuard != nullptr, "Incorrect workerGuard");
    finished = true;
}

std::shared_ptr<NslWorker> makeWorker(TaskManager &taskManager, NsLookup &nsLookup, const Task &task) {
    if (FullWorker::isThisWorker(task.name)) {
        return std::make_shared<FullWorker>(taskManager, nsLookup, task);
    } else if (SimpleWorker::isThisWorker(task.name)) {
        return std::make_shared<SimpleWorker>(taskManager, nsLookup, task);
    } else if (RefreshIpWorker::isThisWorker(task.name)) {
        return std::make_shared<RefreshIpWorker>(taskManager, nsLookup, task);
    } else if (RefreshNodeWorker::isThisWorker(task.name)) {
        return std::make_shared<RefreshNodeWorker>(taskManager, nsLookup, task);
    } else if (FindEmptyNodesWorker::isThisWorker(task.name)) {
        return std::make_shared<FindEmptyNodesWorker>(taskManager, nsLookup, task);
    } else if (PrintNodesWorker::isThisWorker(task.name)) {
        return std::make_shared<PrintNodesWorker>(taskManager, nsLookup, task);
    } else if (MiddleWorker::isThisWorker(task.name)) {
        return std::make_shared<MiddleWorker>(taskManager, nsLookup, task);
    } else {
        throwErr("Incorrect task: " + task.name);
    }
}

} // namespace nslookup
