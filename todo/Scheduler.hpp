#pragma once
#include "TaskManager.hpp"
#include <queue>
#include <thread>       // std::jthread, std::stop_token
#include <stop_token>   // (separate header on MSVC/libc++)
#include <mutex>
#include <chrono>

/**
 * Background engine that pops imminent tasks from a priority-queue snapshot
 * and prints reminders without blocking the main CLI.
 */
class Scheduler {
public:
    explicit Scheduler(TaskManager& taskMgr,
        std::chrono::seconds tick = std::chrono::seconds{ 5 });

    void start();   ///< spawn the jthread (idempotent)
    void stop();    ///< request graceful cancel & join

private:
    // min-heap: earlier deadline → higher priority; if tie, higher enum value
    struct Compare {
        bool operator()(const TaskPtr& a, const TaskPtr& b) const {
            if (a->deadline() == b->deadline())
                return a->priority() < b->priority();   // High > Medium > Low
            return a->deadline() > b->deadline();
        }
    };

    void runLoop(std::stop_token token);    ///< worker entry

    TaskManager& taskManagerRef_;
    std::chrono::seconds    tickInterval_;
    std::jthread            workerThread_;   // auto-joins
    std::mutex              coutMutex_;      // serialize std::cout
};
