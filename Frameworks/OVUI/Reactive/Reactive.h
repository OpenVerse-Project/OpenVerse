#pragma once

#include <Core/Types.h>
#include <functional>
#include <vector>
#include <atomic>
#include <mutex>
#include <unordered_map>
#include <string>
#include <memory>

namespace ovui {

class Subscription {
    std::function<void()> m_unsub;
    bool m_active = true;
public:
    Subscription() = default;
    explicit Subscription(std::function<void()> u) : m_unsub(std::move(u)) {}
    ~Subscription() { unsubscribe(); }
    Subscription(const Subscription&) = delete;
    Subscription& operator=(const Subscription&) = delete;
    Subscription(Subscription&& o) noexcept : m_unsub(std::move(o.m_unsub)), m_active(o.m_active) { o.m_active = false; }
    Subscription& operator=(Subscription&& o) noexcept {
        if (this != &o) { unsubscribe(); m_unsub = std::move(o.m_unsub); m_active = o.m_active; o.m_active = false; }
        return *this;
    }
    void unsubscribe() { if (m_active && m_unsub) { m_unsub(); m_active = false; } }
    bool is_active() const { return m_active; }
};

template<typename T>
class Observable {
    T m_value;
    uint64_t m_version = 0;
    std::vector<std::function<void(const T&)>> m_subscribers;
    mutable std::mutex m_mutex;

public:
    Observable() = default;
    explicit Observable(T v) : m_value(std::move(v)) {}

    const T& get() const { return m_value; }
    uint64_t version() const { return m_version; }

    void set(const T& v) {
        { std::lock_guard lk(m_mutex); if (m_value == v) return; m_value = v; m_version++; }
        notify();
    }

    void set_silent(const T& v) { std::lock_guard lk(m_mutex); m_value = v; m_version++; }

    Subscription subscribe(std::function<void(const T&)> fn) {
        std::lock_guard lk(m_mutex);
        size_t idx = m_subscribers.size();
        m_subscribers.push_back(std::move(fn));
        auto* self = this;
        return Subscription([self, idx]() {
            std::lock_guard lk2(self->m_mutex);
            if (idx < self->m_subscribers.size()) self->m_subscribers[idx] = nullptr;
        });
    }

    operator const T&() const { return get(); }

private:
    void notify() {
        std::vector<std::function<void(const T&)>> copy;
        { std::lock_guard lk(m_mutex); copy = m_subscribers; }
        for (auto& fn : copy) if (fn) fn(m_value);
    }
};

template<typename T>
class Computed {
    T m_cached_value;
    uint64_t m_cached_version = 0;
    std::function<T()> m_compute;
    std::vector<std::function<void(const T&)>> m_subscribers;
    mutable std::mutex m_mutex;
    bool m_dirty = true;

public:
    Computed() = default;
    template<typename Fn> explicit Computed(Fn&& fn) : m_compute(std::forward<Fn>(fn)) {}
    template<typename Fn> void set_compute(Fn&& fn) { m_compute = std::forward<Fn>(fn); m_dirty = true; }

    const T& get() {
        std::lock_guard lk(m_mutex);
        if (m_dirty && m_compute) { m_cached_value = m_compute(); m_cached_version++; m_dirty = false; }
        return m_cached_value;
    }

    void invalidate() { m_dirty = true; }
    operator const T&() { return get(); }
};

class StateStore {
public:
    template<typename T> Observable<T>& state(const std::string& key, T def = {}) {
        auto it = m_store.find(key);
        if (it != m_store.end()) return *static_cast<Observable<T>*>(it->second.get());
        auto obs = std::make_shared<Observable<T>>(std::move(def));
        m_store[key] = obs;
        return *static_cast<Observable<T>*>(obs.get());
    }
    template<typename T> const T& get(const std::string& key, T def = {}) const {
        auto it = m_store.find(key);
        if (it != m_store.end()) return static_cast<Observable<T>*>(it->second.get())->get();
        static T d = def; return d;
    }
    template<typename T> void set(const std::string& key, const T& v) { state<T>(key).set(v); }
    void clear() { m_store.clear(); }
private:
    std::unordered_map<std::string, std::shared_ptr<void>> m_store;
};

} // namespace ovui
