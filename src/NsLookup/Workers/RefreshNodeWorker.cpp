#include "RefreshNodeWorker.h"

#include <functional>

#include "../TaskManager.h"

#include "../NsLookup.h"

#include "check.h"
#include "Log.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

static const std::string TYPE = "RefreshNode_Worker";

static const seconds CONTROL_CHECK_EXPIRE = 30s;

RefreshNodeWorker::RefreshNodeWorker(TaskManager &manager, NsLookup &ns, const Task &task)
    : NslWorker(manager)
    , ns(ns)
{
    CHECK(task.name == TYPE, "Incorrect task on constructor");
    CHECK(task.parameters.canConvert<NsLookupRefreshNodeWorkerTask>(), "Incorrect task RefreshNodeWorker");
    t = task.parameters.value<NsLookupRefreshNodeWorkerTask>();
}

bool RefreshNodeWorker::isThisWorker(const std::string &taskName) {
    return taskName == TYPE;
}

std::string RefreshNodeWorker::getType() const {
    return TYPE;
}

std::string RefreshNodeWorker::getSubType() const {
    return t.node.toStdString();
}

Task RefreshNodeWorker::makeTask(const seconds &remaining, const QString &node) {
    return Task(TYPE, QVariant::fromValue(NsLookupRefreshNodeWorkerTask(node)), remaining);
}

bool RefreshNodeWorker::checkIsActual() const {
    const bool actual1 = checkSpentRecord(CONTROL_CHECK_EXPIRE);
    if (!actual1) {
        LOG << "RefreshNode worker not actual " << t.node;
        return false;
    }

    const size_t countWorked = ns.countWorkedNodes(t.node);
    const bool actual = countWorked == 0;
    if (!actual) {
        LOG << "RefreshNode worker not actual " << t.node;
    }
    return actual;
}

void RefreshNodeWorker::runWorkImpl(WorkerGuard workerGuard) {
    beginWork(workerGuard);
}

void RefreshNodeWorker::beginWork(const WorkerGuard &workerGuard) {
    tt.reset();
    LOG << "RefreshNode worker started " << t.node;

    ns.fillNodeStruct(t.node, node, ipsTemp);

    beginPing(workerGuard, node);
}

void RefreshNodeWorker::beginPing(const WorkerGuard &workerGuard, const NodeType& node) {
    const auto continueResolve = std::bind(&RefreshNodeWorker::continueResolve, this, workerGuard, node);

    ns.continuePing(std::begin(ipsTemp), node, allNodesForTypes, ipsTemp, continueResolve);
}

void RefreshNodeWorker::continueResolve(const WorkerGuard &workerGuard, const NodeType& node) {
    finalizeLookup(workerGuard, node);
}

void RefreshNodeWorker::finalizeLookup(const WorkerGuard &workerGuard, const NodeType& node) {
    ns.finalizeLookup(node.node, allNodesForTypes[node.node]);
    endWork(workerGuard);
}

void RefreshNodeWorker::endWork(const WorkerGuard &workerGuard) {
    addSpentRecord();
    tt.stop();
    LOG << "RefreshNode worker finished. " << t.node << ". Time work: " << tt.countMs();

    finishWork(workerGuard);
}

} // namespace nslookup
