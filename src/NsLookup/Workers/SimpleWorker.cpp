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

static const seconds CONTROL_CHECK = 10min;
static const seconds CONTROL_CHECK_EXPIRE = CONTROL_CHECK + 2min;

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
    const bool actual = now - record.time >= CONTROL_CHECK_EXPIRE;
    if (!actual) {
        LOG << "Simple worker not actual";
    }
    return actual;
}

void SimpleWorker::runWorkImpl(WorkerGuard workerGuard) {
    beginWork(workerGuard);
}

void SimpleWorker::beginWork(const WorkerGuard &workerGuard) {
    tt.reset();
    LOG << "Simple worker started";
    addNewTask(makeTask(CONTROL_CHECK));

    const auto finLookup = std::bind(&SimpleWorker::finalizeLookup, this, workerGuard);
    const auto beginPing = std::bind(&SimpleWorker::beginPing, this, workerGuard, _1);

    auto tmp = std::map<NodeType::Node, std::vector<NodeInfo>>();
    ns.beginResolve(tmp, ipsTemp, finLookup, beginPing);
}

void SimpleWorker::beginPing(const WorkerGuard &workerGuard, std::map<QString, NodeType>::const_iterator node) {
    const auto continueResolve = std::bind(&SimpleWorker::continueResolve, this, workerGuard, node);

    ns.continuePingSafe(node->second, ipsTemp, continueResolve);
}

void SimpleWorker::continueResolve(const WorkerGuard &workerGuard, std::map<QString, NodeType>::const_iterator node) {
    const auto finalizeLookup = std::bind(&SimpleWorker::finalizeLookup, this, workerGuard);
    const auto beginPing = std::bind(&SimpleWorker::beginPing, this, workerGuard, _1);

    auto tmp = std::map<NodeType::Node, std::vector<NodeInfo>>();
    ns.continueResolve(std::next(node), tmp, ipsTemp, finalizeLookup, beginPing);
}

void SimpleWorker::finalizeLookup(const WorkerGuard &workerGuard) {
    ns.saveAll(false);
    endWork(workerGuard);
}

void SimpleWorker::endWork(const WorkerGuard &workerGuard) {
    addSpentRecord();

    tt.stop();
    LOG << "Simple worker finished. Time work: " << tt.countMs();

    finishWork(workerGuard);
}

} // namespace nslookup
