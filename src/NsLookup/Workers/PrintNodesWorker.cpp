#include "PrintNodesWorker.h"

#include <functional>

#include "../TaskManager.h"

#include "../NsLookup.h"

#include "check.h"
#include "Log.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

static const std::string TYPE = "PrintNodes_Worker";
static const std::string SUB_TYPE = "All";

static const seconds REPEAT_CHECK = 1min;

PrintNodesWorker::PrintNodesWorker(TaskManager &manager, NsLookup &ns, const Task &task)
    : NslWorker(manager)
    , ns(ns)
{
    CHECK(task.name == TYPE, "Incorrect task on constructor");
}

bool PrintNodesWorker::isThisWorker(const std::string &taskName) {
    return taskName == TYPE;
}

std::string PrintNodesWorker::getType() const {
    return TYPE;
}

std::string PrintNodesWorker::getSubType() const {
    return SUB_TYPE;
}

Task PrintNodesWorker::makeTask(const seconds &remaining) {
    return Task(TYPE, QVariant(), remaining);
}

bool PrintNodesWorker::checkIsActual() const {
    return true;
}

void PrintNodesWorker::runWorkImpl(WorkerGuard workerGuard) {
    tt.reset();
    beginWork(workerGuard);
}

void PrintNodesWorker::beginWork(const WorkerGuard &workerGuard) {
    addNewTask(makeTask(REPEAT_CHECK));

    ns.printNodes();

    endWork(workerGuard);
}

void PrintNodesWorker::endWork(const WorkerGuard &workerGuard) {
    tt.stop();

    finishWork(workerGuard);
}

} // namespace nslookup
