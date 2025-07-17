#pragma once
#pragma execution_character_set("utf-8")
#include <string>
#include <chrono>
#include <memory>

// Simple three-level priority enum
enum class Priority : int { Low = 0, Medium, High };

// Base Task 
/**
 * Polymorphic base for one-shot and specialized tasks.
 * Holds title, deadline, priority, and "already reminded?" flag.
 */
class Task {
public:
    Task(std::string  title,
        std::chrono::system_clock::time_point deadline,
        Priority pr = Priority::Medium);

    virtual ~Task() = default;

    // getters 
    const std::string& title()    const noexcept;
    std::chrono::system_clock::time_point deadline() const noexcept;
    Priority                              priority() const noexcept;

    // virtual hooks 
    virtual bool        isRecurring() const noexcept { return false; }
    virtual void        reschedule() { /* default NOP */ }
    virtual std::string info() const;                // CLI-friendly string

    //  reminder flag helpers 
    bool  notified()      const noexcept { return notified_; }
    void  markNotified() { notified_ = true; }
    void  resetNotified() { notified_ = false; }

    // setters 
    void setTitle(std::string newTitle) { title_ = std::move(newTitle); }
    void setPriority(Priority p) { priority_ = p; }
    void setDeadline(std::chrono::system_clock::time_point tp) {
        deadline_ = tp;
        resetNotified();       // moving date → re-enable reminder
    }

protected:
    void shiftDeadline(std::chrono::system_clock::duration delta);

private:
    std::string                             title_;
    std::chrono::system_clock::time_point   deadline_;
    Priority                                priority_;
    bool                                    notified_ = false;
};

using TaskPtr = std::shared_ptr<Task>;

//  RecurringTask 
class RecurringTask : public Task {
public:
    RecurringTask(std::string  title,
        std::chrono::minutes interval,
        Priority pr = Priority::Medium);

    bool isRecurring() const noexcept override { return true; }
    void reschedule() override;     // shift deadline by interval
    std::string info() const override;

private:
    std::chrono::minutes interval_;
};

// MeetingTask 
class MeetingTask : public Task {
public:
    MeetingTask(std::string title,
        std::chrono::system_clock::time_point deadline,
        std::string location,
        Priority pr = Priority::Medium);

    std::string info() const override;

private:
    std::string location_;
};

// ShoppingTask 
class ShoppingTask : public Task {
public:
    using Task::Task;                      // inherit ctor
    std::string info() const override { return "🛒 " + Task::info(); }
};
