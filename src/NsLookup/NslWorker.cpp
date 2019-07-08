#include "NslWorker.h"

#include "duration.h"
#include "check.h"
#include "Log.h"

#include "TaskManager.h"

#include "Workers/FullWorker.h"

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
    } else {
        throwErr("Incorrect task: " + task.name);
    }
}

} // namespace nslookup
