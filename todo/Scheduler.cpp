#pragma execution_character_set("utf-8")
#include "Scheduler.hpp"
#include <iostream>
#include <thread>  // std::this_thread::sleep_for

// ───────────────────────────────────────── constructor
Scheduler::Scheduler(TaskManager& taskMgr, std::chrono::seconds tick)
    : taskManagerRef_(taskMgr), tickInterval_(tick) {}

// ───────────────────────────────────────── start/stop
void Scheduler::start() {
    if (workerThread_.joinable()) return;   // already running

    // Launch jthread; lambda captures `this` and forwards stop_token
    workerThread_ = std::jthread([this](std::stop_token st) {
        runLoop(st);
        });
}

void Scheduler::stop() {
    if (workerThread_.joinable()) {
        workerThread_.request_stop();   // cooperative cancel
        workerThread_.join();           // wait for loop to exit
    }
}

// ───────────────────────────────────────── main worker loop
void Scheduler::runLoop(std::stop_token stopTok) {
    // Helper: final pass before thread exits
    auto flushReminders = [this] {
        auto snapshot = taskManagerRef_.snapshot();
        auto now = std::chrono::system_clock::now();

        for (const auto& task : snapshot) {
            auto diffMin =
                std::chrono::duration_cast<std::chrono::minutes>(task->deadline() - now);
            if (!task->notified() && diffMin.count() <= 5 && diffMin.count() >= -5)
                std::cout << "⏰  Reminder: " << task->info() << '\n';
        }
        };

    while (!stopTok.stop_requested()) {
        // Build local min-heap snapshot (thread-safe copy)
        std::priority_queue<TaskPtr, std::vector<TaskPtr>, Compare> queue;
        for (const auto& task : taskManagerRef_.snapshot()) queue.push(task);

        while (!queue.empty()) {
            auto task = queue.top();
            auto now = std::chrono::system_clock::now();
            auto diff = task->deadline() - now;

            // 0. skip already-notified tasks
            if (task->notified()) { queue.pop(); continue; }

            // 1. earliest task >5 min away → nothing to do
            if (diff > std::chrono::minutes{ 5 }) break;

            // 2. within [0,5] min window → remind once
            if (diff >= std::chrono::minutes{ 0 }) {
                {
                    std::scoped_lock lk(coutMutex_);
                    std::cout << "⏰  Reminder: " << task->info() << '\n';
                }
                task->markNotified();
                queue.pop();

                if (task->isRecurring()) {
                    task->reschedule();
                    queue.push(task);      // push next occurrence
                }
                continue;
            }

            // 3. already overdue → drop
            queue.pop();
        }

        // Sleep for tickInterval_ or until stop requested
        std::this_thread::sleep_for(tickInterval_);
    }

    flushReminders();  // graceful shutdown
}
