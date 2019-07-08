#include "RefreshIpWorker.h"

#include <functional>

#include "../TaskManager.h"

#include "../NsLookup.h"

#include "check.h"
#include "Log.h"

using namespace std::placeholders;

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

static const std::string TYPE = "RefreshIp_Worker";

RefreshIpWorker::RefreshIpWorker(TaskManager &manager, NsLookup &ns, const Task &task)
    : NslWorker(manager)
    , ns(ns)
{
    CHECK(task.name == TYPE, "Incorrect task on constructor");
    CHECK(task.parameters.canConvert<NsLookupRefreshIpWorkerTask>(), "Incorrect task RefreshIpWorker");
    t = task.parameters.value<NsLookupRefreshIpWorkerTask>();
}

bool RefreshIpWorker::isThisWorker(const std::string &taskName) {
    return taskName == TYPE;
}

std::string RefreshIpWorker::getType() const {
    return TYPE;
}

std::string RefreshIpWorker::getSubType() const {
    return t.address.toStdString();
}

Task RefreshIpWorker::makeTask(const seconds &remaining, const QString &address, size_t counter) {
    return Task(TYPE, QVariant::fromValue(NsLookupRefreshIpWorkerTask(address, counter)), remaining);
}

bool RefreshIpWorker::checkIsActual() const {
    const size_t currentCounter = ns.findCountUpdatedIp(t.address);
    const bool actual = currentCounter == t.counter;
    if (!actual) {
        LOG << "RefreshIp worker not actual " << t.address;
    }
    return actual;
}

void RefreshIpWorker::runWorkImpl(WorkerGuard workerGuard) {
    beginWork(workerGuard);
}

void RefreshIpWorker::beginWork(const WorkerGuard &workerGuard) {
    tt.reset();
    LOG << "RefreshIp worker started " << t.address;
    const auto beginPing = std::bind(&RefreshIpWorker::beginPing, this, workerGuard, _1);

    ns.processRefreshIp(t.address, ipsTemp, beginPing);
}

void RefreshIpWorker::beginPing(const WorkerGuard &workerGuard, const NodeType& node) {
    const auto continueResolve = std::bind(&RefreshIpWorker::continueResolve, this, workerGuard, node);

    ns.continuePing(std::begin(ipsTemp), node, allNodesForTypes, ipsTemp, continueResolve);
}

void RefreshIpWorker::continueResolve(const WorkerGuard &workerGuard, const NodeType& node) {
    finalizeLookup(workerGuard, node);
}

void RefreshIpWorker::finalizeLookup(const WorkerGuard &workerGuard, const NodeType& node) {
    ns.finalizeRefreshIp(node.node, allNodesForTypes);
    endWork(workerGuard);
}

void RefreshIpWorker::endWork(const WorkerGuard &workerGuard) {
    tt.stop();
    LOG << "RefreshIp worker finished. " << t.address << ". Time work: " << tt.countMs();

    finishWork(workerGuard);
}

} // namespace nslookup
