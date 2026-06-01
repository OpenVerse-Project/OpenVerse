#pragma once

#include <Core/Types.h>
#include <Reactive/Reactive.h>
#include <functional>
#include <vector>
#include <string>

namespace ovui {

enum class EasingType {
    Linear, EaseIn, EaseOut, EaseInOut,
    EaseInQuad, EaseOutQuad, EaseInOutQuad,
    EaseInCubic, EaseOutCubic, EaseInOutCubic,
    EaseInElastic, EaseOutElastic, EaseInOutElastic,
    EaseInBounce, EaseOutBounce, EaseInOutBounce,
    EaseInBack, EaseOutBack,
    Spring,
    Custom
};

class EasingCurve {
public:
    EasingCurve(EasingType type = EasingType::EaseInOut);
    static EasingCurve custom(std::function<float(float)> fn);
    float evaluate(float t) const;
    EasingType type() const { return m_type; }

private:
    EasingType m_type;
    std::function<float(float)> m_custom;
    static float spring_eval(float t);
    static float bounce_eval(float t);
};

struct Keyframe {
    float time;
    float value;
    EasingCurve easing;
};

struct KeyframeTrack {
    std::string property;
    std::vector<Keyframe> keyframes;
    float evaluate(float t) const;
    float duration() const;
    void sort();
};

class Timeline {
public:
    Timeline();

    void add_track(const KeyframeTrack& track);
    KeyframeTrack& track_at(size_t i) { return m_tracks[i]; }
    const std::vector<KeyframeTrack>& tracks() const { return m_tracks; }
    std::vector<KeyframeTrack>& tracks() { return m_tracks; }
    size_t track_count() const { return m_tracks.size(); }
    void clear();

    void set_loop(bool loop) { m_loop = loop; }
    bool is_looping() const { return m_loop; }

    void set_duration(float d) { m_duration_override = d; }
    float duration() const;

    void play();
    void pause();
    void stop();
    void seek(float t);

    float current_time() const { return m_time; }
    bool is_playing() const { return m_playing; }
    bool is_finished() const { return m_finished; }

    void advance(float dt);
    float evaluate(const std::string& property, float default_val = 0) const;

    std::function<void()> on_finished;

private:
    std::vector<KeyframeTrack> m_tracks;
    float m_time = 0;
    float m_duration_override = -1;
    bool m_playing = false;
    bool m_loop = false;
    bool m_finished = false;
};

struct SpringConfig {
    float stiffness = 200.0f;
    float damping = 20.0f;
    float mass = 1.0f;
    float initial_velocity = 0.0f;
    float precision = 0.001f;
};

class SpringAnimation {
public:
    SpringAnimation();

    void configure(const SpringConfig& cfg);
    void set_target(float target);
    void set_value(float v);

    float current() const { return m_current; }
    float velocity() const { return m_velocity; }
    float target() const { return m_target; }
    bool is_resting() const { return m_resting; }

    void advance(float dt);
    Observable<float>& observable() { return m_obs; }

private:
    SpringConfig m_cfg;
    float m_current = 0;
    float m_velocity = 0;
    float m_target = 0;
    bool m_resting = true;
    Observable<float> m_obs{0};
};

class AnimationController {
public:
    AnimationController();

    SpringAnimation& spring(const std::string& name);
    Timeline& timeline(const std::string& name);

    void advance_all(float dt);

    void clear();

private:
    std::unordered_map<std::string, std::unique_ptr<SpringAnimation>> m_springs;
    std::unordered_map<std::string, std::unique_ptr<Timeline>> m_timelines;
};

} // namespace ovui
