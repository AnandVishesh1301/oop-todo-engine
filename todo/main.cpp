#include "TaskManager.hpp"
#include "Scheduler.hpp"
#include "AutoSave.hpp"
#include "Storage.hpp"

#pragma execution_character_set("utf-8")

#include <iostream>
#include <sstream>
#include <format>
#include <chrono>



using namespace std::chrono;

// ───────────────────────────── helper funcs
Priority parsePriority(std::string_view sv) {
    if (sv == "High" || sv == "high")   return Priority::High;
    if (sv == "Medium" || sv == "medium") return Priority::Medium;
    return Priority::Low;
}

std::chrono::system_clock::time_point
parseDateTime(const std::string& date, const std::string& time)
{
    std::istringstream in(date + ' ' + time);
    std::chrono::system_clock::time_point tp;
    in >> std::chrono::parse("%F %R", tp);   // %F=YYYY-MM-DD, %R=HH:MM
    if (in.fail()) throw std::runtime_error("bad date/time format");
    return tp;
}

// ───────────────────────────── CLI help banner
void printHelp() {
    std::cout << R"(Commands:
/* Core create */
  add           "Title" YYYY-MM-DD HH:MM Priority
  add-recurring "Title" <minutes> Priority
  add-meeting   "Title" YYYY-MM-DD HH:MM Priority Location

/* Modify / delete */
  del  "Title"
  edit "Title" [new-title] [YYYY-MM-DD HH:MM] [Priority]

/* House-keeping */
  list
  clear
  quit

Priorities: Low | Medium | High
Examples:
  add  "Pay rent" 2025-08-01 09:00 High
  del  "Pay rent"
  edit "Pay rent" 2025-08-02 08:00 Medium
  edit "Pay rent" 'Rent (moved)' High

New Note: The Time Zone is in UTC
    )";
}

// ───────────────────────────── entry point
int main() try {
    TaskManager taskManager;
    Storage     storage{ "tasks.json" };
    AutoSave    saver{ storage, taskManager };   // RAII snapshot
    storage.load(taskManager);

    Scheduler scheduler{ taskManager };
    scheduler.start();

    std::cout << "Simple TODO (type help)\n";

    // ── main CLI loop
    for (std::string line; std::getline(std::cin, line); ) {
        std::istringstream iss(line);
        std::string command;   iss >> command;

        // -------------- static commands --------------
        if (command == "help") { printHelp(); continue; }
        if (command == "quit")  break;

        if (command == "list") {
            for (auto& task : taskManager.tasks())
                std::cout << "- " << task->info() << '\n';
            continue;
        }
        if (command == "clear") {
            taskManager.clear();
            std::cout << "All tasks removed.\n";
            continue;
        }

        // -------------- del / edit need quoted title --------------
        if (command == "del" || command == "edit") {
            std::string quotedTitle;
            iss >> std::ws;
            if (iss.peek() == '"') { iss.get(); std::getline(iss, quotedTitle, '"'); }

            if (quotedTitle.empty()) { std::cout << "missing quoted title\n"; continue; }

            if (command == "del") {
                if (taskManager.removeTask(quotedTitle))
                    std::cout << "Deleted \"" << quotedTitle << "\"\n";
                else
                    std::cout << "Task not found\n";
                continue;
            }

            // -------- edit --------
            auto task = taskManager.findTask(quotedTitle);
            if (!task) { std::cout << "Task not found\n"; continue; }

            // optional args: new-title, date+time, priority
            std::string newTitle;
            std::string date, time, priorityStr;

            iss >> std::ws;
            if (iss.peek() == '"') { iss.get(); std::getline(iss, newTitle, '"'); }

            iss >> date;
            if (std::isdigit(date[0])) {        // user entered date
                iss >> time >> priorityStr;
            }
            else {                            // date omitted; maybe priority
                priorityStr = date;
                date.clear();
            }

            if (!newTitle.empty()) task->setTitle(newTitle);
            if (!date.empty()) {
                try { task->setDeadline(parseDateTime(date, time)); }
                catch (...) { std::cout << "bad date/time – edit skipped\n"; }
            }
            if (!priorityStr.empty()) task->setPriority(parsePriority(priorityStr));

            std::cout << "Edited: " << task->info() << '\n';
            continue;
        }

        // -------------- create commands need quoted title --------------
        std::string title;
        iss >> std::ws;
        if (iss.peek() == '"') { iss.get(); std::getline(iss, title, '"'); }

        if (title.empty()) { std::cout << "missing quoted title\n"; continue; }

        // -------- add one-shot --------
        if (command == "add") {
            std::string date, time, priorityStr;
            iss >> date >> time >> priorityStr;
            try {
                auto tp = parseDateTime(date, time);
                taskManager.addTask(std::make_shared<Task>(title, tp,
                    parsePriority(priorityStr)));
                auto now = std::chrono::system_clock::now();
                std::cout << "Time Now: " << std::format("{:%Y-%m-%d %H:%M}", now) << '\n';
            }
            catch (const std::exception& e) { std::cout << e.what() << '\n'; }
            continue;
        }

        // -------- add recurring --------
        if (command == "add-recurring") {
            int minutes; std::string priorityStr;
            iss >> minutes >> priorityStr;
            taskManager.addTask(std::make_shared<RecurringTask>(
                title, std::chrono::minutes{ minutes }, parsePriority(priorityStr)));

            auto now = std::chrono::system_clock::now();
            std::cout << "Time Now: " << std::format("{:%Y-%m-%d %H:%M}", now) << '\n';
            continue;
        }

        // -------- add meeting --------
        if (command == "add-meeting") {
            std::string date, time, priorityStr, location;
            iss >> date >> time >> priorityStr;
            std::getline(iss >> std::ws, location);  // remainder is location
            try {
                auto tp = parseDateTime(date, time);
                taskManager.addTask(std::make_shared<MeetingTask>(
                    title, tp, location, parsePriority(priorityStr)));

                auto now = std::chrono::system_clock::now();
                std::cout << "Time Now: " << std::format("{:%Y-%m-%d %H:%M}", now) << '\n';
            }
            catch (const std::exception& e) { std::cout << e.what() << '\n'; }
            continue;
        }

        std::cout << "unknown command\n";
    }

    storage.save(taskManager);
    scheduler.stop();
}
catch (const std::exception& e) {
    std::cerr << "Fatal: " << e.what() << '\n';
    return 1;
}
