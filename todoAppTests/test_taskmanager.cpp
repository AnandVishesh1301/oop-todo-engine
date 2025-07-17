#include "pch.h"
#include "gtest/gtest.h"
#include "../todo/TaskManager.hpp"   
#include <chrono>

using namespace std::chrono_literals;

// Helper: make a one-shot task 2 min from now
static TaskPtr makeTask(std::string title, Priority pr = Priority::Medium) {
    auto tp = std::chrono::system_clock::now() + 2min;
    return std::make_shared<Task>(std::move(title), tp, pr);
}

TEST(TaskManagerBasics, AddAndSnapshot) {
    TaskManager mgr;

    // add two tasks
    auto t1 = mgr.addTask(makeTask("First", Priority::High));
    auto t2 = mgr.addTask(makeTask("Second", Priority::Low));

    // the vector inside should now have size 2
    EXPECT_EQ(mgr.tasks().size(), 2);

    // snapshot returns *copies*; original remains intact after erase()
    auto snap = mgr.snapshot();
    snap.clear();
    EXPECT_EQ(mgr.tasks().size(), 2);

    // remove one -> size 1
    EXPECT_TRUE(mgr.removeTask("First"));
    EXPECT_EQ(mgr.tasks().size(), 1);
}
