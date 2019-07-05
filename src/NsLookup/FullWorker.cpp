#include "FullWorker.h"

#include "TaskManager.h"

#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

static const std::string TYPE = "Full_Worker";
static const std::string SUB_TYPE = "Full";

static const seconds CONTROL_CHECK = 10min;
static const seconds CONTROL_CHECK_EXPIRE = CONTROL_CHECK + 5min;
static const seconds REPEAT_CHECK = hours(24);

FullWorker::FullWorker(TaskManager &manager, NsLookup &ns, const Task &task)
    : NslWorker(manager)
    , ns(ns)
{
    CHECK(task.name == TYPE, "Incorrect task on constructor");
}

bool FullWorker::isThisWorker(const std::string &taskName) {
    return taskName == TYPE;
}

std::string FullWorker::getType() const {
    return TYPE;
}

std::string FullWorker::getSubType() const {
    return SUB_TYPE;
}

Task FullWorker::makeTask(const seconds &remaining) {
    return Task(TYPE, QVariant(), remaining);
}

bool FullWorker::checkIsActual() const {
    TaskRecord record;
    const bool foundSpent = findSpentRecord(record);
    if (!foundSpent) {
        return true;
    }
    CHECK(record.type == TYPE && record.subtype == SUB_TYPE, "Incorrect Task Record");
    const system_time_point now = ::system_now();
    return now - record.time >= CONTROL_CHECK_EXPIRE;
}

void FullWorker::runWorkImpl(WorkerGuard workerGuard) {
    beginWork(workerGuard);
}

void FullWorker::beginWork(const WorkerGuard &workerGuard) {
    addNewTask(makeTask(CONTROL_CHECK));
}

void FullWorker::endWork(const WorkerGuard &workerGuard) {
    addNewTask(makeTask(REPEAT_CHECK));
}

} // namespace nslookup
