#pragma once
#include "TaskManager.hpp"
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

/// Increment if/when the JSON schema changes.
inline constexpr int STORAGE_VERSION = 1;

/**
 * Handles JSON persistence:
 *   • atomic write-then-rename for crash safety
 *   • .bak fallback on corruption
 */
class Storage {
public:
    explicit Storage(fs::path filePath);

    void save(const TaskManager& manager) const;  ///< may throw
    void load(TaskManager& manager) const;        ///< may throw

private:
    fs::path filePath_;   ///< e.g. "tasks.json"
};
