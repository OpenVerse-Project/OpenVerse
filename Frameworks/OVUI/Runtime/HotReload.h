#pragma once

#include <Core/Types.h>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <filesystem>
#include <memory>

namespace ovui {

struct WatchedFile {
    std::filesystem::path path;
    std::filesystem::file_time_type last_write;
    bool dirty = false;
};

enum class HotReloadAction {
    ReloadOVML,
    ReloadOVSS,
    ReloadFont,
    Custom
};

struct HotReloadHook {
    HotReloadAction action;
    std::string file_extension;
    std::function<void(const std::string& path)> callback;
};

class HotReloadEngine {
public:
    HotReloadEngine();

    void add_watch(const std::string& path);
    void add_watch_recursive(const std::string& directory, const std::string& extension);
    void remove_watch(const std::string& path);

    void add_hook(HotReloadHook hook);
    void clear_hooks();

    void poll(float interval_ms = 100.0f);
    void start_async(float interval_ms = 250.0f);
    void stop_async();

    bool is_running() const { return m_running; }
    size_t watch_count() const { return m_watches.size(); }
    size_t hook_count() const { return m_hooks.size(); }

    const std::vector<std::string>& changed_files() const { return m_changed_files; }

    std::function<void(const std::string& path, HotReloadAction action)> on_file_changed;

private:
    std::vector<WatchedFile> m_watches;
    std::vector<HotReloadHook> m_hooks;
    mutable std::vector<std::string> m_changed_files;
    bool m_running = false;
    std::unique_ptr<std::thread> m_thread;

    void check_changes();
    void trigger_hooks(const std::string& path, const std::string& ext);
};

class DifferentialRebuilder {
public:
    DifferentialRebuilder();

    void add_source(const std::string& key, const std::string& source);
    void remove_source(const std::string& key);

    bool has_changed(const std::string& key, const std::string& new_source);
    void update(const std::string& key, const std::string& new_source);

    std::vector<std::string> changed_keys() const;
    void clear();

private:
    struct SourceEntry {
        std::string source;
        size_t hash;
    };
    std::unordered_map<std::string, SourceEntry> m_sources;
    mutable std::vector<std::string> m_changed;

    static size_t hash_string(const std::string& s);
};

} // namespace ovui
