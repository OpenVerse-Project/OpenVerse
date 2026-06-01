#include "Animation.h"
#include <cmath>
#include <algorithm>
#include <unordered_map>

namespace ovui {

// ============================================================
// EasingCurve
// ============================================================
EasingCurve::EasingCurve(EasingType type) : m_type(type) {}

EasingCurve EasingCurve::custom(std::function<float(float)> fn) {
    EasingCurve c(EasingType::Custom);
    c.m_custom = std::move(fn);
    return c;
}

float EasingCurve::spring_eval(float t) {
    float decay = std::exp(-6.0f * t);
    return 1.0f - decay * std::cos(12.0f * t);
}

float EasingCurve::bounce_eval(float t) {
    float n1 = 7.5625f, d1 = 2.75f;
    if (t < 1.0f / d1) return n1 * t * t;
    if (t < 2.0f / d1) { t -= 1.5f / d1; return n1 * t * t + 0.75f; }
    if (t < 2.5f / d1) { t -= 2.25f / d1; return n1 * t * t + 0.9375f; }
    t -= 2.625f / d1;
    return n1 * t * t + 0.984375f;
}

float EasingCurve::evaluate(float t) const {
    t = std::clamp(t, 0.0f, 1.0f);
    switch (m_type) {
        case EasingType::Linear: return t;
        case EasingType::EaseIn: return t * t;
        case EasingType::EaseOut: return 1.0f - (1.0f - t) * (1.0f - t);
        case EasingType::EaseInOut: return t < 0.5f ? 2.0f * t * t : 1.0f - std::pow(-2.0f * t + 2.0f, 2.0f) * 0.5f;
        case EasingType::EaseInQuad: return t * t;
        case EasingType::EaseOutQuad: return t * (2.0f - t);
        case EasingType::EaseInOutQuad: return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
        case EasingType::EaseInCubic: return t * t * t;
        case EasingType::EaseOutCubic: { float t1 = t - 1.0f; return t1 * t1 * t1 + 1.0f; }
        case EasingType::EaseInOutCubic: return t < 0.5f ? 4.0f * t * t * t : (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f;
        case EasingType::EaseInElastic: { if (t == 0 || t == 1) return t; return -std::pow(2.0f, 10.0f * t - 10.0f) * std::sin((t * 10.0f - 10.75f) * 2.094f); }
        case EasingType::EaseOutElastic: { if (t == 0 || t == 1) return t; return std::pow(2.0f, -10.0f * t) * std::sin((t * 10.0f - 0.75f) * 2.094f) + 1.0f; }
        case EasingType::EaseInOutElastic: {
            if (t == 0 || t == 1) return t;
            if (t < 0.5f) return -(std::pow(2.0f, 20.0f * t - 10.0f) * std::sin((20.0f * t - 11.125f) * 1.396f)) * 0.5f;
            return std::pow(2.0f, -20.0f * t + 10.0f) * std::sin((20.0f * t - 11.125f) * 1.396f) * 0.5f + 1.0f;
        }
        case EasingType::EaseInBounce: return 1.0f - bounce_eval(1.0f - t);
        case EasingType::EaseOutBounce: return bounce_eval(t);
        case EasingType::EaseInOutBounce: return t < 0.5f ? (1.0f - bounce_eval(1.0f - 2.0f * t)) * 0.5f : (1.0f + bounce_eval(2.0f * t - 1.0f)) * 0.5f;
        case EasingType::EaseInBack: { float c = 1.70158f; return (c + 1.0f) * t * t * t - c * t * t; }
        case EasingType::EaseOutBack: { float c = 1.70158f; float t1 = t - 1.0f; return t1 * t1 * ((c + 1.0f) * t1 + c) + 1.0f; }
        case EasingType::Spring: return spring_eval(t);
        case EasingType::Custom: return m_custom ? m_custom(t) : t;
    }
    return t;
}

// ============================================================
// KeyframeTrack
// ============================================================
float KeyframeTrack::evaluate(float t) const {
    if (keyframes.empty()) return 0;
    if (keyframes.size() == 1) return keyframes[0].value;

    for (size_t i = 0; i < keyframes.size() - 1; i++) {
        if (t >= keyframes[i].time && t <= keyframes[i + 1].time) {
            float local_t = (t - keyframes[i].time) / (keyframes[i + 1].time - keyframes[i].time + 0.0001f);
            float eased = keyframes[i + 1].easing.evaluate(local_t);
            return keyframes[i].value + (keyframes[i + 1].value - keyframes[i].value) * eased;
        }
    }
    return t <= keyframes[0].time ? keyframes[0].value : keyframes.back().value;
}

float KeyframeTrack::duration() const {
    return keyframes.empty() ? 0 : keyframes.back().time;
}

void KeyframeTrack::sort() {
    std::sort(keyframes.begin(), keyframes.end(),
              [](auto& a, auto& b) { return a.time < b.time; });
}

// ============================================================
// Timeline
// ============================================================
Timeline::Timeline() {}

void Timeline::add_track(const KeyframeTrack& track) {
    m_tracks.push_back(track);
    m_tracks.back().sort();
}

void Timeline::clear() { m_tracks.clear(); m_time = 0; m_finished = false; }

float Timeline::duration() const {
    if (m_duration_override > 0) return m_duration_override;
    float d = 0;
    for (auto& t : m_tracks) d = std::max(d, t.duration());
    return d;
}

void Timeline::play() { m_playing = true; m_finished = false; }
void Timeline::pause() { m_playing = false; }
void Timeline::stop() { m_playing = false; m_time = 0; m_finished = false; }
void Timeline::seek(float t) { m_time = t; }

void Timeline::advance(float dt) {
    if (!m_playing) return;
    m_time += dt;
    float dur = duration();
    if (m_time >= dur) {
        if (m_loop) { m_time -= dur; }
        else { m_time = dur; m_playing = false; m_finished = true; if (on_finished) on_finished(); }
    }
}

float Timeline::evaluate(const std::string& property, float default_val) const {
    for (auto& t : m_tracks) {
        if (t.property == property) return t.evaluate(m_time);
    }
    return default_val;
}

// ============================================================
// SpringAnimation
// ============================================================
SpringAnimation::SpringAnimation() { m_obs.set_silent(0); }

void SpringAnimation::configure(const SpringConfig& cfg) { m_cfg = cfg; }

void SpringAnimation::set_target(float target) {
    m_target = target;
    m_resting = false;
}

void SpringAnimation::set_value(float v) {
    m_current = v;
    m_obs.set_silent(v);
}

void SpringAnimation::advance(float dt) {
    if (m_resting) return;

    float force = -m_cfg.stiffness * (m_current - m_target);
    float damping_force = -m_cfg.damping * m_velocity;
    float acceleration = (force + damping_force) / m_cfg.mass;

    m_velocity += acceleration * dt;
    m_current += m_velocity * dt;

    if (std::abs(m_velocity) < m_cfg.precision && std::abs(m_current - m_target) < m_cfg.precision) {
        m_current = m_target;
        m_velocity = 0;
        m_resting = true;
    }

    m_obs.set_silent(m_current);
}

// ============================================================
// AnimationController
// ============================================================
AnimationController::AnimationController() {}

SpringAnimation& AnimationController::spring(const std::string& name) {
    auto it = m_springs.find(name);
    if (it != m_springs.end()) return *it->second;
    auto ptr = std::make_unique<SpringAnimation>();
    auto& ref = *ptr;
    m_springs[name] = std::move(ptr);
    return ref;
}

Timeline& AnimationController::timeline(const std::string& name) {
    auto it = m_timelines.find(name);
    if (it != m_timelines.end()) return *it->second;
    auto ptr = std::make_unique<Timeline>();
    auto& ref = *ptr;
    m_timelines[name] = std::move(ptr);
    return ref;
}

void AnimationController::advance_all(float dt) {
    for (auto& [_, s] : m_springs) s->advance(dt);
    for (auto& [_, t] : m_timelines) t->advance(dt);
}

void AnimationController::clear() { m_springs.clear(); m_timelines.clear(); }

} // namespace ovui
