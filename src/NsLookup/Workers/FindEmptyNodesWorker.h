#ifndef FINDEMPTYNODESWORKER_H
#define FINDEMPTYNODESWORKER_H

#include <map>
#include <vector>

#include "../NslWorker.h"
#include "../NsLookupStructs.h"

#include "duration.h"

class NsLookup;

namespace nslookup {

class FindEmptyNodesWorker final: public NslWorker {
public:
    FindEmptyNodesWorker(TaskManager &manager, NsLookup &ns, const Task &task);

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

    void endWork(const WorkerGuard &workerGuard);

private:

    NsLookup &ns;

    Timer tt;

};

} // namespace nslookup

#endif // FINDEMPTYNODESWORKER_H
