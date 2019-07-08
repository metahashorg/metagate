#ifndef FULLWORKER_H
#define FULLWORKER_H

#include <map>
#include <vector>

#include "../NslWorker.h"
#include "../NsLookupStructs.h"

#include "duration.h"

class NsLookup;

namespace nslookup {

class FullWorker final: public NslWorker {
public:
    FullWorker(TaskManager &manager, NsLookup &ns, const Task &task);

public:

    static bool isThisWorker(const std::string &taskName);

    static Task makeTask(const seconds &remaining);

protected:

    std::string getType() const override;

    std::string getSubType() const override;

    bool checkIsActual() const override;

    void runWorkImpl(WorkerGuard workerGuard) override;

private:

    void beginWork(const WorkerGuard &workerGuard);

    void beginPing(const WorkerGuard &workerGuard, std::map<QString, NodeType>::const_iterator node);

    void continueResolve(const WorkerGuard &workerGuard, std::map<QString, NodeType>::const_iterator node);

    void finalizeLookup(const WorkerGuard &workerGuard);

    void endWork(const WorkerGuard &workerGuard);

private:

    NsLookup &ns;

    std::map<NodeType::Node, std::vector<NodeInfo>> allNodesForTypes;

    std::vector<QString> ipsTemp;

};

} // namespace nslookup

#endif // FULLWORKER_H
