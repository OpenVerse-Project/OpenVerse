#pragma once

#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Animation/Animation.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ovui {

class TimelineEditor : public Container {
public:
    TimelineEditor();
    ~TimelineEditor() override = default;

    void set_timeline(std::shared_ptr<Timeline> tl);
    std::shared_ptr<Timeline> timeline() const { return m_timeline; }

    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    void set_track_height(float h) { m_track_h = std::max(16.0f, h); }
    float track_height() const { return m_track_h; }
    void set_header_width(float w) { m_header_w = std::max(40.0f, w); }
    void set_ruler_height(float h) { m_ruler_h = std::max(16.0f, h); }
    void set_zoom(float z) { m_zoom = std::clamp(z, 0.1f, 100.0f); }
    float zoom() const { return m_zoom; }

    void set_playhead(float t);
    float playhead() const { return m_playhead; }

    float time_from_x(float x) const;
    float x_from_time(float t) const;

    std::function<void(float)> on_playhead_changed;
    std::function<void(const std::string& track, int idx, float t, float v)> on_keyframe_changed;
    std::function<void(const std::string& track, int idx)> on_keyframe_selected;

    bool is_empty() const;

private:
    std::shared_ptr<Timeline> m_timeline;
    float m_track_h = 24;
    float m_header_w = 80;
    float m_ruler_h = 24;
    float m_zoom = 1.0f;
    float m_playhead = 0;

    struct SelectedKf { std::string track; int index = -1; };
    SelectedKf m_selected_kf;
    bool m_dragging_playhead = false;
    bool m_dragging_kf = false;
    Point m_drag_start;

    struct TrackLayout { std::string name; float y; };
    std::vector<TrackLayout> m_track_layout;

    void draw_ruler(PaintContext& ctx, Rect area) const;
    void draw_track(PaintContext& ctx, const KeyframeTrack& track, float y, float track_area_x, float track_w) const;
    void draw_playhead(PaintContext& ctx, Rect area) const;
    void draw_keyframe(PaintContext& ctx, float x, float y, float r, Color c, bool selected) const;
};

class CurveEditor : public Container {
public:
    CurveEditor();
    ~CurveEditor() override = default;

    void set_curve(const EasingCurve& curve);
    const EasingCurve& curve() const { return m_curve; }

    void set_easing_type(EasingType type);

    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    void set_padding(float p) { m_pad = p; }
    void set_control_point_radius(float r) { m_cp_radius = r; }

    std::function<void(EasingType)> on_curve_changed;

private:
    EasingCurve m_curve{EasingType::EaseInOut};
    float m_pad = 16;
    float m_cp_radius = 6;
    Point m_cp1{0.25f, 0.1f};
    Point m_cp2{0.25f, 1.0f};
    int m_drag_cp = -1;

    void draw_background(PaintContext& ctx, Rect area) const;
    void draw_curve(PaintContext& ctx, Rect area) const;
    void draw_control_points(PaintContext& ctx, Rect area) const;
    Point canvas_to_normalized(Point p, Rect area) const;
    Point normalized_to_canvas(Point p, Rect area) const;
};

} // namespace ovui
