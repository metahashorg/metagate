#include "TaskManager.h"

#include "NslWorker.h"

#include "check.h"
#include "Log.h"

SET_LOG_NAMESPACE("NSL");

namespace nslookup {

const static size_t MAX_SPENT_TASKS = 100;

Task::Task(const std::string &name, const QVariant &parameters, const seconds &remaining)
    : name(name)
    , parameters(parameters)
    , execTime1(system_now() + remaining)
    , execTime2(now() + remaining)
{}

bool Task::operator>(const Task &second) const {
    return execTime1 < second.execTime1;
}

bool Task::isTime(const system_time_point &tp1, const time_point &tp2) const {
    return execTime1 >= tp1 || execTime2 >= tp2;
}

TaskRecord::TaskRecord() = default;

TaskRecord::TaskRecord(const std::string &type, const std::string &subtype, const system_time_point &time)
    : type(type)
    , subtype(subtype)
    , time(time)
{}

void TaskManager::addTask(const Task &task) {
    tasks.push(task);
}

bool TaskManager::isTaskReady() const {
    if (tasks.empty()) {
        return false;
    }
    const Task &task = tasks.top();
    const system_time_point snow = ::system_now();
    const time_point now = ::now();
    return task.isTime(snow, now);
}

Task TaskManager::popTask() {
    CHECK(!tasks.empty(), "Empty tasks queue");
    Task task = tasks.top();
    tasks.pop();
    return task;
}

bool TaskManager::isCurrentWork() const {
    return !currentWorker.expired();
}

void TaskManager::runWork(std::shared_ptr<NslWorker> worker) {
    currentWorker = worker;
    worker->runWork(worker);
}

void TaskManager::addSpentTask(const TaskRecord &task) {
    spentTasks.emplace_front(task);
    spentTasks.erase(spentTasks.begin() + std::min(MAX_SPENT_TASKS, spentTasks.size()), spentTasks.end());
}

bool TaskManager::findSpentTask(const std::string &type, const std::string &subtype, TaskRecord &result) {
    const auto found = std::find_if(spentTasks.begin(), spentTasks.end(), [&type, &subtype](const TaskRecord &r) {
        return r.type == type && r.subtype == subtype;
    });
    if (found == spentTasks.end()) {
        return false;
    } else {
        result = *found;
        return true;
    }
}

} // namespace nslookup
