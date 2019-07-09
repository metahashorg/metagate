#include "MiddleWorker.h"

#include <functional>

#include "../TaskManager.h"

#include "../NsLookup.h"

#include "check.h"
#include "Log.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

static const std::string TYPE = "Middle_Worker";
static const std::string SUB_TYPE = "Middle";

static const seconds REPEAT_CHECK = 5min;
static const seconds REPEAT_CHECK_EXPIRE = REPEAT_CHECK - 2min;

MiddleWorker::MiddleWorker(TaskManager &manager, NsLookup &ns, const Task &task)
    : NslWorker(manager)
    , ns(ns)
{
    CHECK(task.name == TYPE, "Incorrect task on constructor");
}

bool MiddleWorker::isThisWorker(const std::string &taskName) {
    return taskName == TYPE;
}

std::string MiddleWorker::getType() const {
    return TYPE;
}

std::string MiddleWorker::getSubType() const {
    return SUB_TYPE;
}

Task MiddleWorker::makeTask(const seconds &remaining) {
    return Task(TYPE, QVariant(), remaining);
}

bool MiddleWorker::checkIsActual() const {
    TaskRecord record;
    const bool foundSpent = findSpentRecord(record);
    if (!foundSpent) {
        return true;
    }
    CHECK(record.type == TYPE && record.subtype == SUB_TYPE, "Incorrect Task Record");
    const system_time_point now = ::system_now();
    const bool actual = now - record.time >= REPEAT_CHECK_EXPIRE;
    if (!actual) {
        LOG << "Middle worker not actual";
    }
    return actual;
}

void MiddleWorker::runWorkImpl(WorkerGuard workerGuard) {
    beginWork(workerGuard);
}

void MiddleWorker::beginWork(const WorkerGuard &workerGuard) {
    tt.reset();
    LOG << "Middle worker started";
    addNewTask(makeTask(REPEAT_CHECK));

    std::map<QString, NodeType>::const_iterator node = ns.getBeginNodesIterator();
    const bool isEnd = !ns.fillNodeStruct(node, ipsTemp);
    if (isEnd) {
        endWork(workerGuard);
        return;
    }
    beginPing(workerGuard, node);
}

void MiddleWorker::beginPing(const WorkerGuard &workerGuard, std::map<QString, NodeType>::const_iterator node) {
    const auto continueResolve = std::bind(&MiddleWorker::continueResolve, this, workerGuard, node);

    ns.continuePing(std::begin(ipsTemp), node->second, allNodesForTypes, ipsTemp, continueResolve);
}

void MiddleWorker::continueResolve(const WorkerGuard &workerGuard, std::map<QString, NodeType>::const_iterator node) {
    node = std::next(node);
    ipsTemp.clear();
    const bool isEnd = !ns.fillNodeStruct(node, ipsTemp);
    if (isEnd) {
        finalizeLookup(workerGuard);
        return;
    }
    beginPing(workerGuard, node);
}

void MiddleWorker::finalizeLookup(const WorkerGuard &workerGuard) {
    ns.finalizeLookup(false, allNodesForTypes);
    endWork(workerGuard);
}

void MiddleWorker::endWork(const WorkerGuard &workerGuard) {
    addSpentRecord();

    tt.stop();
    LOG << "Middle worker finished. Time work: " << tt.countMs();

    finishWork(workerGuard);
}

} // namespace nslookup
