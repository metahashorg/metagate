#ifndef REFRESHNODEWORKER_H
#define REFRESHNODEWORKER_H

#include <map>
#include <vector>

#include <QString>
#include <QMetaType>

#include "../NslWorker.h"
#include "../NsLookupStructs.h"

#include "duration.h"

class NsLookup;

struct NsLookupRefreshNodeWorkerTask {
    QString node;

    NsLookupRefreshNodeWorkerTask() = default;

    NsLookupRefreshNodeWorkerTask(const QString &node)
        : node(node)
    {}
};

Q_DECLARE_METATYPE(NsLookupRefreshNodeWorkerTask)

namespace nslookup {

class RefreshNodeWorker final: public NslWorker {
public:
    RefreshNodeWorker(TaskManager &manager, NsLookup &ns, const Task &task);

public:

    static bool isThisWorker(const std::string &taskName);

    static Task makeTask(const seconds &remaining, const QString &node);

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

    NsLookupRefreshNodeWorkerTask t;

    NodeType node;

};

} // namespace nslookup

#endif // REFRESHNODEWORKER_H
