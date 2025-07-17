#include "Task.hpp"

#pragma execution_character_set("utf-8")
#include <format>
#include <chrono>

// ───────────────────────────────────────── Base Task
Task::Task(std::string title,
    std::chrono::system_clock::time_point deadline,
    Priority pr)
    : title_(std::move(title)), deadline_(deadline), priority_(pr) {}

const std::string& Task::title()    const noexcept { return title_; }
std::chrono::system_clock::time_point Task::deadline() const noexcept { return deadline_; }
Priority Task::priority()           const noexcept { return priority_; }

void Task::shiftDeadline(std::chrono::system_clock::duration delta) {
    deadline_ += delta;
}

std::string Task::info() const {
    return std::format("{} [prio {}] @{}",
        title_,
        static_cast<int>(priority_),
        std::format("{:%Y-%m-%d %H:%M}", deadline()));
}

// ───────────────────────────────────────── RecurringTask
RecurringTask::RecurringTask(std::string title,
    std::chrono::minutes interval,
    Priority pr)
    : Task(std::move(title),
        std::chrono::system_clock::now() + interval,
        pr),
    interval_(interval) {}

void RecurringTask::reschedule() {
    shiftDeadline(interval_);
    resetNotified();
}

std::string RecurringTask::info() const {
    return "🔁 " + Task::info() +
        std::format(" every {} min", interval_.count());
}

// ───────────────────────────────────────── MeetingTask
MeetingTask::MeetingTask(std::string title,
    std::chrono::system_clock::time_point tp,
    std::string location,
    Priority pr)
    : Task(std::move(title), tp, pr), location_(std::move(location)) {}

std::string MeetingTask::info() const {
    return "📅 " + Task::info() + " at " + location_;
}
