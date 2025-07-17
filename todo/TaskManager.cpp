#include "TaskManager.hpp"

#pragma execution_character_set("utf-8")
#include <algorithm>

// writer lock: add task
TaskPtr TaskManager::addTask(const TaskPtr& task) {
    std::unique_lock lk(listMutex_);
    taskList_.push_back(task);
    return task;
}

// reader lock: find by title
TaskPtr TaskManager::findTask(std::string_view title) {
    std::shared_lock lk(listMutex_);
    for (auto& task : taskList_)
        if (task->title() == title) return task;
    return nullptr;
}

// writer lock: remove by title
bool TaskManager::removeTask(const std::string& title) {
    std::unique_lock lk(listMutex_);
    auto it = std::remove_if(taskList_.begin(), taskList_.end(),
        [&](const TaskPtr& t) { return t->title() == title; });
    bool removed = it != taskList_.end();
    taskList_.erase(it, taskList_.end());
    return removed;
}

// reader lock: copy out full list
std::vector<TaskPtr> TaskManager::tasks() const {
    std::shared_lock lk(listMutex_);
    return taskList_;
}

std::vector<TaskPtr> TaskManager::snapshot() const { return tasks(); }

// writer lock: remove everything
void TaskManager::clear() {
    std::unique_lock lk(listMutex_);
    taskList_.clear();
}
