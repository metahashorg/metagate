#ifndef MIDDLEWORKER_H
#define MIDDLEWORKER_H

#include <map>
#include <vector>

#include "../NslWorker.h"
#include "../NsLookupStructs.h"

#include "duration.h"

class NsLookup;

namespace nslookup {

class MiddleWorker final: public NslWorker {
public:
    MiddleWorker(TaskManager &manager, NsLookup &ns, const Task &task);

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

    Timer tt;

};

} // namespace nslookup

#endif // MIDDLEWORKER_H
