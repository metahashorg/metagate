#ifndef NSLWORKER_H
#define NSLWORKER_H

#include <string>
#include <memory>

class NsLookup;

namespace nslookup {

class TaskManager;
struct TaskRecord;
struct Task;

class NslWorker;

using WorkerGuard = std::shared_ptr<NslWorker>;

class NslWorker {
public:

    NslWorker(TaskManager &manager);

    virtual ~NslWorker();

public:

    bool isActual() const;

    void runWork(WorkerGuard workerGuard);

protected:

    virtual std::string getType() const = 0;

    virtual std::string getSubType() const = 0;

    virtual bool checkIsActual() const = 0;

    virtual void runWorkImpl(WorkerGuard workerGuard) = 0;

protected:

    void addNewTask(const Task &task);

    void addSpentRecord();

    bool findSpentRecord(TaskRecord &result) const;

    // Нужен в основном для того, чтобы убедиться что workerGuard не потерялся
    void finishWork(WorkerGuard workerGuard);

private:

    TaskManager &manager;

    bool finished = true;
};

std::shared_ptr<NslWorker> makeWorker(TaskManager &taskManager, NsLookup &nsLookup, const Task &task);

} // namespace nslookup

#endif // NSLWORKER_H
