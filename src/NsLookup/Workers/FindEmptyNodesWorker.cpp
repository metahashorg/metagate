#include "FindEmptyNodesWorker.h"

#include <functional>

#include "../TaskManager.h"

#include "../NsLookup.h"

#include "check.h"
#include "Log.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

static const std::string TYPE = "FindEmptyNodes_Worker";
static const std::string SUB_TYPE = "All";

static const seconds REPEAT_CHECK = 40s;

FindEmptyNodesWorker::FindEmptyNodesWorker(TaskManager &manager, NsLookup &ns, const Task &task)
    : NslWorker(manager)
    , ns(ns)
{
    CHECK(task.name == TYPE, "Incorrect task on constructor");
}

bool FindEmptyNodesWorker::isThisWorker(const std::string &taskName) {
    return taskName == TYPE;
}

std::string FindEmptyNodesWorker::getType() const {
    return TYPE;
}

std::string FindEmptyNodesWorker::getSubType() const {
    return SUB_TYPE;
}

Task FindEmptyNodesWorker::makeTask(const seconds &remaining) {
    return Task(TYPE, QVariant(), remaining);
}

bool FindEmptyNodesWorker::checkIsActual() const {
    return true;
}

void FindEmptyNodesWorker::runWorkImpl(WorkerGuard workerGuard) {
    beginWork(workerGuard);
}

void FindEmptyNodesWorker::beginWork(const WorkerGuard &workerGuard) {
    tt.reset();
    LOG << "FindEmptyNodes worker started";
    addNewTask(makeTask(REPEAT_CHECK));

    ns.findAndRefreshEmptyNodes();

    endWork(workerGuard);
}

void FindEmptyNodesWorker::endWork(const WorkerGuard &workerGuard) {
    tt.stop();
    LOG << "FindEmptyNodes worker finished. Time work: " << tt.countMs();

    finishWork(workerGuard);
}

} // namespace nslookup
