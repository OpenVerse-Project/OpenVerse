#include "HotReload.h"
#include <algorithm>

namespace ovui {

HotReloadEngine::HotReloadEngine() {}

void HotReloadEngine::add_watch(const std::string& path_str) {
    std::filesystem::path p(path_str);
    if (!std::filesystem::exists(p)) return;
    WatchedFile wf{p, std::filesystem::last_write_time(p), false};
    m_watches.push_back(wf);
}

void HotReloadEngine::add_watch_recursive(const std::string& directory, const std::string& extension) {
    std::filesystem::path dir(directory);
    if (!std::filesystem::exists(dir)) return;

    for (auto& entry : std::filesystem::recursive_directory_iterator(dir)) {
        if (entry.is_regular_file() && entry.path().extension() == extension)
            add_watch(entry.path().string());
    }
}

void HotReloadEngine::remove_watch(const std::string& path) {
    std::filesystem::path p(path);
    m_watches.erase(
        std::remove_if(m_watches.begin(), m_watches.end(),
                       [&](auto& w) { return w.path == p; }),
        m_watches.end()
    );
}

void HotReloadEngine::add_hook(HotReloadHook hook) {
    m_hooks.push_back(std::move(hook));
}

void HotReloadEngine::clear_hooks() { m_hooks.clear(); }

void HotReloadEngine::check_changes() {
    m_changed_files.clear();

    for (auto& w : m_watches) {
        if (!std::filesystem::exists(w.path)) continue;
        auto current = std::filesystem::last_write_time(w.path);
        if (current != w.last_write) {
            w.last_write = current;
            w.dirty = true;
            m_changed_files.push_back(w.path.string());
        }
    }
}

void HotReloadEngine::trigger_hooks(const std::string& path, const std::string& ext) {
    for (auto& hook : m_hooks) {
        if (ext == hook.file_extension || hook.file_extension == "*") {
            if (hook.callback) hook.callback(path);
            if (on_file_changed) on_file_changed(path, hook.action);
        }
    }
}

void HotReloadEngine::poll(float interval_ms) {
    check_changes();
    for (auto& path : m_changed_files) {
        std::string ext = std::filesystem::path(path).extension().string();
        trigger_hooks(path, ext);
    }
    if (interval_ms > 0) {
        std::this_thread::sleep_for(
            std::chrono::microseconds(static_cast<int64_t>(interval_ms * 1000)));
    }
}

void HotReloadEngine::start_async(float interval_ms) {
    m_running = true;
    m_thread = std::make_unique<std::thread>([this, interval_ms]() {
        while (m_running) poll(interval_ms);
    });
}

void HotReloadEngine::stop_async() {
    m_running = false;
    if (m_thread && m_thread->joinable()) m_thread->join();
    m_thread.reset();
}

// ============================================================
// DifferentialRebuilder
// ============================================================
DifferentialRebuilder::DifferentialRebuilder() {}

size_t DifferentialRebuilder::hash_string(const std::string& s) {
    size_t h = 0;
    for (char c : s) h = h * 31 + (size_t)(unsigned char)c;
    return h;
}

void DifferentialRebuilder::add_source(const std::string& key, const std::string& source) {
    m_sources[key] = {source, hash_string(source)};
}

void DifferentialRebuilder::remove_source(const std::string& key) {
    m_sources.erase(key);
}

bool DifferentialRebuilder::has_changed(const std::string& key, const std::string& new_source) {
    auto it = m_sources.find(key);
    if (it == m_sources.end()) return true;
    return hash_string(new_source) != it->second.hash;
}

void DifferentialRebuilder::update(const std::string& key, const std::string& new_source) {
    size_t h = hash_string(new_source);
    auto it = m_sources.find(key);
    if (it == m_sources.end() || it->second.hash != h) {
        m_sources[key] = {new_source, h};
        m_changed.push_back(key);
    }
}

std::vector<std::string> DifferentialRebuilder::changed_keys() const {
    auto keys = m_changed;
    m_changed.clear();
    return keys;
}

void DifferentialRebuilder::clear() { m_sources.clear(); m_changed.clear(); }

} // namespace ovui
