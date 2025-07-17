#include "Storage.hpp"

#pragma execution_character_set("utf-8")
#include <fstream>
#include <chrono>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
namespace fs = std::filesystem;

// ───────────────────────────────────────── constructor
Storage::Storage(fs::path filePath) : filePath_(std::move(filePath)) {}

// ───────────────────────────────────────── save
void Storage::save(const TaskManager& manager) const {
    json root;
    root["version"] = STORAGE_VERSION;
    root["tasks"] = json::array();

    // Serialize tasks
    for (const auto& task : manager.tasks()) {
        root["tasks"].push_back({
            { "title",     task->title() },
            { "deadline",  std::chrono::duration_cast<std::chrono::seconds>(
                               task->deadline().time_since_epoch()).count() },
            { "priority",  static_cast<int>(task->priority()) },
            { "recurring", task->isRecurring() }
            });
    }

    // Write to temp → rename (atomic)
    fs::path tmp = filePath_.string() + ".tmp";
    {
        std::ofstream ofs(tmp, std::ios::trunc);
        if (!ofs) throw std::runtime_error("Cannot open " + tmp.string());
        ofs << root.dump(2);
    }

    fs::path bak = filePath_.string() + ".bak";
    std::error_code ec;
    if (fs::exists(filePath_, ec)) fs::rename(filePath_, bak, ec);  // keep backup
    fs::rename(tmp, filePath_, ec);
    if (ec) throw std::runtime_error("Atomic rename failed: " + ec.message());
}

// ───────────────────────────────────────── load
void Storage::load(TaskManager& manager) const {
    auto tryLoad = [&](const fs::path& p) -> bool {
        std::ifstream ifs(p);
        if (!ifs) return false;

        json root; ifs >> root;
        if (!root.contains("version") || root["version"] != STORAGE_VERSION)
            return false;   // wrong schema

        for (const auto& entry : root["tasks"]) {
            auto secs = std::chrono::seconds{ entry.at("deadline").get<long long>() };
            auto tp = std::chrono::system_clock::time_point{ secs };

            manager.addTask(std::make_shared<Task>(
                entry.at("title").get<std::string>(),
                tp,
                static_cast<Priority>(entry.at("priority").get<int>())));
        }
        return true;
        };

    fs::path bak = filePath_.string() + ".bak";
    if (tryLoad(filePath_)) return;   // primary
    if (tryLoad(bak))       return;   // fallback

    throw std::runtime_error("Storage::load failed: corrupt or missing JSON");
}
