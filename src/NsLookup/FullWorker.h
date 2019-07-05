#ifndef FULLWORKER_H
#define FULLWORKER_H

#include "NslWorker.h"

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

    void endWork(const WorkerGuard &workerGuard);

private:

    NsLookup &ns;

};

} // namespace nslookup

#endif // FULLWORKER_H
