#pragma once
#include "Task.hpp"
#include <string_view>
#include <vector>
#include <shared_mutex>   

/**
 * Thread-safe in-memory registry for all tasks.
 * Provides CRUD plus snapshotting for the Scheduler.
 */
class TaskManager {
public:
    TaskPtr addTask(const TaskPtr& task);
    TaskPtr findTask(std::string_view title);   // returns nullptr if not found
    bool    removeTask(const std::string& title);

    std::vector<TaskPtr> tasks()    const;      // copy out (reader lock)
    std::vector<TaskPtr> snapshot() const;      // alias for readability
    void clear();                               // remove all tasks (writer lock)

private:
    std::vector<TaskPtr>      taskList_;
    mutable std::shared_mutex listMutex_;       // many readers, single writer
};
