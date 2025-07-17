#pragma once
#include <iostream>
#include "Storage.hpp"

/**
 * RAII helper: persists all tasks when this object goes out of scope
 * (e.g., normal exit, exception, or early return).
 */

class AutoSave {
public:
    AutoSave(Storage& storageRef, const TaskManager& managerRef)
        : storage_(storageRef), manager_(managerRef) {}

    ~AutoSave() noexcept {
        try { storage_.save(manager_); }
        catch (...) { std::cerr << "AutoSave failed\n"; }
    }

private:
    Storage& storage_;   /// reference to JSON storage handler
    const TaskManager& manager_;   /// const ref to current task registry
};
