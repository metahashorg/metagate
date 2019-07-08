#ifndef REFRESHIPWORKER_H
#define REFRESHIPWORKER_H

#include <map>
#include <vector>

#include <QString>
#include <QMetaType>

#include "../NslWorker.h"
#include "../NsLookupStructs.h"

#include "duration.h"

class NsLookup;

struct NsLookupRefreshIpWorkerTask {
    QString address;
    size_t counter;

    NsLookupRefreshIpWorkerTask() = default;

    NsLookupRefreshIpWorkerTask(const QString &address, size_t counter)
        : address(address)
        , counter(counter)
    {}
};

Q_DECLARE_METATYPE(NsLookupRefreshIpWorkerTask)

namespace nslookup {

class RefreshIpWorker final: public NslWorker {
public:
    RefreshIpWorker(TaskManager &manager, NsLookup &ns, const Task &task);

public:

    static bool isThisWorker(const std::string &taskName);

    static Task makeTask(const seconds &remaining, const QString &address, size_t counter);

protected:

    std::string getType() const override;

    std::string getSubType() const override;

    bool checkIsActual() const override;

    void runWorkImpl(WorkerGuard workerGuard) override;

private:

    void beginWork(const WorkerGuard &workerGuard);

    void beginPing(const WorkerGuard &workerGuard, const NodeType& node);

    void continueResolve(const WorkerGuard &workerGuard, const NodeType& node);

    void finalizeLookup(const WorkerGuard &workerGuard, const NodeType& node);

    void endWork(const WorkerGuard &workerGuard);

private:

    NsLookup &ns;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypes;

    std::vector<QString> ipsTemp;

    Timer tt;

    NsLookupRefreshIpWorkerTask t;

};

} // namespace nslookup

#endif // REFRESHIPWORKER_H
