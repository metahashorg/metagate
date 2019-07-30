#ifndef TASKMANAGER_H
#define TASKMANAGER_H

#include <string>
#include <queue>
#include <memory>

#include <QVariant>

#include "duration.h"

namespace nslookup {

class NslWorker;

struct Task {
    std::string name;
    QVariant parameters;

    system_time_point execTime1;
    time_point execTime2;

    Task(const std::string &name, const QVariant &parameters, const seconds &remaining);

    bool operator>(const Task &second) const;

    bool isTime(const system_time_point &tp1, const time_point &tp2) const;
};

struct TaskRecord {
    std::string type;
    std::string subtype;
    system_time_point time;

    TaskRecord();

    TaskRecord(const std::string &type, const std::string &subtype, const system_time_point &time);
};

class TaskManager {
    friend class NslWorker;
public:

    void addTask(const Task &task);

    bool isTaskReady() const;

    Task popTask();

    bool isCurrentWork() const;

    void runWork(std::shared_ptr<NslWorker> worker);

protected:

    void addSpentTask(const TaskRecord &task);

    bool findSpentTask(const std::string &type, const std::string &subtype, TaskRecord &result);

    void resetCurrentWork();

private:

    std::weak_ptr<NslWorker> currentWorker;

    std::priority_queue<Task, std::vector<Task>, std::greater<Task>> tasks;

    std::deque<TaskRecord> spentTasks;
};

} // namespace nslookup

#endif // TASKMANAGER_H
