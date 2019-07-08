#ifndef PRINTNODESWORKER_H
#define PRINTNODESWORKER_H

#include <map>
#include <vector>

#include "../NslWorker.h"
#include "../NsLookupStructs.h"

#include "duration.h"

class NsLookup;

namespace nslookup {

class PrintNodesWorker final: public NslWorker {
public:
    PrintNodesWorker(TaskManager &manager, NsLookup &ns, const Task &task);

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

};

} // namespace nslookup

#endif // PRINTNODESWORKER_H
