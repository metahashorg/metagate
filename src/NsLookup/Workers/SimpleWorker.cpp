#include "SimpleWorker.h"

#include <functional>

#include "../TaskManager.h"

#include "../NsLookup.h"

#include "check.h"
#include "Log.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

static const std::string TYPE = "Simple_Worker";
static const std::string SUB_TYPE = "Simple";

static const seconds REPEAT_CHECK = 10min;
static const seconds REPEAT_CHECK_EXPIRE = REPEAT_CHECK - 1min;

SimpleWorker::SimpleWorker(TaskManager &manager, NsLookup &ns, const Task &task)
    : NslWorker(manager)
    , ns(ns)
{
    CHECK(task.name == TYPE, "Incorrect task on constructor");
}

bool SimpleWorker::isThisWorker(const std::string &taskName) {
    return taskName == TYPE;
}

std::string SimpleWorker::getType() const {
    return TYPE;
}

std::string SimpleWorker::getSubType() const {
    return SUB_TYPE;
}

Task SimpleWorker::makeTask(const seconds &remaining) {
    return Task(TYPE, QVariant(), remaining);
}

bool SimpleWorker::checkIsActual() const {
    TaskRecord record;
    const bool foundSpent = findSpentRecord(record);
    if (!foundSpent) {
        return true;
    }
    CHECK(record.type == TYPE && record.subtype == SUB_TYPE, "Incorrect Task Record");
    const system_time_point now = ::system_now();
    const bool actual = now - record.time >= REPEAT_CHECK_EXPIRE;
    if (!actual) {
        LOG << "Simple worker not actual";
    }
    return actual;
}

void SimpleWorker::runWorkImpl(WorkerGuard workerGuard) {
    tt.reset();
    LOG << "Simple worker started";
    beginWork(workerGuard);
}

void SimpleWorker::beginWork(const WorkerGuard &workerGuard) {
    addNewTask(makeTask(REPEAT_CHECK));

    const auto finLookup = std::bind(&SimpleWorker::finalizeLookup, this, workerGuard);
    const auto beginPing = std::bind(&SimpleWorker::beginPing, this, workerGuard, _1);

    ns.beginResolve(allNodesForTypes, ipsTemp, finLookup, beginPing);
}

void SimpleWorker::beginPing(const WorkerGuard &workerGuard, std::map<QString, NodeType>::const_iterator node) {
    const auto continueResolve = std::bind(&SimpleWorker::continueResolve, this, workerGuard, node);

    ns.continuePing(true, std::begin(ipsTemp), node->second, allNodesForTypes, ipsTemp, continueResolve);
}

void SimpleWorker::continueResolve(const WorkerGuard &workerGuard, std::map<QString, NodeType>::const_iterator node) {
    const auto finalizeLookup = std::bind(&SimpleWorker::finalizeLookup, this, workerGuard);
    const auto beginPing = std::bind(&SimpleWorker::beginPing, this, workerGuard, _1);

    ns.continueResolve(std::next(node), allNodesForTypes, ipsTemp, finalizeLookup, beginPing);
}

void SimpleWorker::finalizeLookup(const WorkerGuard &workerGuard) {
    const auto endWork = std::bind(&SimpleWorker::endWork, this, workerGuard);

    ns.finalizeLookup(allNodesForTypes, endWork);
}

void SimpleWorker::endWork(const WorkerGuard &workerGuard) {
    addSpentRecord();

    tt.stop();
    LOG << "Simple worker finished. Time work: " << tt.countMs();

    finishWork(workerGuard);
}

} // namespace nslookup
