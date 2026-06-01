#include "AnimationTimeline.h"
#include <algorithm>
#include <cmath>

namespace ovui {

TimelineEditor::TimelineEditor() {
    set_widget_type("TimelineEditor");
    layout_node().direction = FlexDirection::Column;
}

void TimelineEditor::set_timeline(std::shared_ptr<Timeline> tl) {
    m_timeline = std::move(tl);
}

void TimelineEditor::set_playhead(float t) {
    m_playhead = std::max(0.0f, t);
}

float TimelineEditor::time_from_x(float x) const {
    Rect f = frame();
    float track_x = f.x + m_header_w;
    float track_w = std::max(1.0f, f.width - m_header_w);
    float dur = m_timeline ? std::max(0.01f, m_timeline->duration()) : 1.0f;
    float visible = dur / m_zoom;
    return (x - track_x) / track_w * visible;
}

float TimelineEditor::x_from_time(float t) const {
    Rect f = frame();
    float track_x = f.x + m_header_w;
    float track_w = std::max(1.0f, f.width - m_header_w);
    float dur = m_timeline ? std::max(0.01f, m_timeline->duration()) : 1.0f;
    float visible = dur / m_zoom;
    return track_x + (t / visible) * track_w;
}

bool TimelineEditor::is_empty() const {
    return !m_timeline || m_timeline->duration() <= 0;
}

void TimelineEditor::draw_ruler(PaintContext& ctx, Rect area) const {
    float ruler_y = area.y;
    float ruler_h = m_ruler_h;

    ctx.draw_rect({area.x + m_header_w, ruler_y, area.width - m_header_w, ruler_h},
                  Color{0.12f, 0.12f, 0.15f, 1}, 0);

    float dur = m_timeline ? std::max(0.01f, m_timeline->duration()) : 5.0f;
    float visible = dur / m_zoom;

    float tick_step = 0.1f;
    if (visible > 1.0f) tick_step = 0.5f;
    if (visible > 5.0f) tick_step = 1.0f;
    if (visible > 15.0f) tick_step = 5.0f;
    if (visible > 60.0f) tick_step = 10.0f;
    if (visible > 120.0f) tick_step = 30.0f;
    if (visible > 300.0f) tick_step = 60.0f;

    for (float t = 0; t <= dur + tick_step; t += tick_step) {
        float x = x_from_time(t);
        if (x > area.x + area.width) break;
        float tick_h = (fmod(t, tick_step * 5) < 0.001f) ? ruler_h * 0.6f : ruler_h * 0.3f;
        ctx.draw_line({x, ruler_y + ruler_h - tick_h}, {x, ruler_y + ruler_h},
                      Color{0.6f, 0.6f, 0.6f, 0.5f}, 1);
    }

    ctx.draw_border({area.x + m_header_w, ruler_y, area.width - m_header_w, ruler_h},
                    Color{0.3f, 0.3f, 0.35f, 1}, 1, 0);
}

void TimelineEditor::draw_track(PaintContext& ctx, const KeyframeTrack& track,
                                 float y, float track_x, float track_w) const {
    float lane_y = y + m_ruler_h;
    Color track_bg = Color{0.16f, 0.16f, 0.2f, 1};

    ctx.draw_rect({track_x, lane_y, track_w, m_track_h}, track_bg, 0);
    ctx.draw_border({track_x, lane_y, track_w, m_track_h}, Color{0.25f, 0.25f, 0.3f, 1}, 1, 0);

    for (size_t i = 0; i < track.keyframes.size(); i++) {
        auto& kf = track.keyframes[i];
        float kf_x = x_from_time(kf.time);
        if (kf_x >= track_x - 10 && kf_x <= track_x + track_w + 10) {
            bool sel = (m_selected_kf.index == (int)i && m_selected_kf.track == track.property);
            draw_keyframe(ctx, kf_x, lane_y + m_track_h * 0.5f, 5, Color{0.31f, 0.76f, 0.97f, 1}, sel);
        }
    }
}

void TimelineEditor::draw_playhead(PaintContext& ctx, Rect area) const {
    float px = x_from_time(m_playhead);
    float top = area.y + m_ruler_h;
    float bottom = area.y + area.height;
    ctx.draw_line({px, top}, {px, bottom}, Color{1, 0.2f, 0.2f, 0.9f}, 2);
}

void TimelineEditor::draw_keyframe(PaintContext& ctx, float x, float y, float r, Color c, bool selected) const {
    ctx.draw_circle({x, y}, selected ? r * 1.5f : r, c);
    if (selected) {
        ctx.draw_border({x - r * 1.5f, y - r * 1.5f, r * 3, r * 3}, Color{1, 1, 1, 1}, 1, r * 1.5f);
    }
}

void TimelineEditor::on_paint(PaintContext& ctx) {
    Rect f = frame();
    if (f.width <= 0 || f.height <= 0) return;

    draw_ruler(ctx, f);

    m_track_layout.clear();
    if (!m_timeline) return;

    float track_x = f.x + m_header_w;
    float track_w = std::max(1.0f, f.width - m_header_w);

    for (auto& tk : m_timeline->tracks()) {
        float lane_y = m_ruler_h + m_track_layout.size() * m_track_h;
        m_track_layout.push_back({tk.property, lane_y});

        ctx.draw_rect({f.x, f.y + lane_y, m_header_w, m_track_h},
                      Color{0.14f, 0.14f, 0.18f, 1}, 0);
        ctx.draw_text(tk.property, {f.x + 4, f.y + lane_y + 4},
                      Color{0.7f, 0.7f, 0.75f, 1}, 11);
        ctx.draw_border({f.x, f.y + lane_y, m_header_w, m_track_h},
                        Color{0.25f, 0.25f, 0.3f, 1}, 1, 0);

        draw_track(ctx, tk, f.y, track_x, track_w);
    }

    draw_playhead(ctx, f);
}

bool TimelineEditor::on_event(InputEvent& ev) {
    Rect f = frame();
    if (!f.contains(ev.pos) && !m_dragging_playhead && !m_dragging_kf) return false;

    if (ev.type == EventType::MouseDown) {
        if (ev.pos.y < f.y + m_ruler_h) {
            m_dragging_playhead = true;
            m_playhead = time_from_x(ev.pos.x);
            if (on_playhead_changed) on_playhead_changed(m_playhead);
            return true;
        }

        if (!m_timeline) return false;

        auto& tk_list = m_timeline->tracks();
        m_selected_kf = {};
        for (auto& t : tk_list) {
            for (size_t i = 0; i < t.keyframes.size(); i++) {
                float kf_x = x_from_time(t.keyframes[i].time);
                if (std::abs(ev.pos.x - kf_x) < 8) {
                    m_selected_kf = {t.property, (int)i};
                    m_dragging_kf = true;
                    m_drag_start = ev.pos;
                    if (on_keyframe_selected) on_keyframe_selected(t.property, (int)i);
                    return true;
                }
            }
        }

        m_playhead = time_from_x(ev.pos.x);
        if (on_playhead_changed) on_playhead_changed(m_playhead);
        return true;
    }

    if (ev.type == EventType::MouseMove) {
        if (m_dragging_playhead) {
            m_playhead = time_from_x(ev.pos.x);
            if (on_playhead_changed) on_playhead_changed(m_playhead);
            return true;
        }
        if (m_dragging_kf && m_selected_kf.index >= 0 && m_timeline) {
            float new_t = time_from_x(ev.pos.x);
            for (auto& t : m_timeline->tracks()) {
                if (t.property == m_selected_kf.track && m_selected_kf.index < (int)t.keyframes.size()) {
                    auto& kf = t.keyframes[m_selected_kf.index];
                    kf.time = std::max(0.0f, new_t);
                    if (on_keyframe_changed)
                        on_keyframe_changed(m_selected_kf.track, m_selected_kf.index, kf.time, kf.value);
                }
            }
            return true;
        }
    }

    if (ev.type == EventType::MouseUp) {
        m_dragging_playhead = false;
        m_dragging_kf = false;
        return m_dragging_playhead || m_dragging_kf;
    }

    return false;
}

CurveEditor::CurveEditor() {
    set_widget_type("CurveEditor");
}

void CurveEditor::set_curve(const EasingCurve& curve) {
    m_curve = curve;
}

void CurveEditor::set_easing_type(EasingType type) {
    m_curve = EasingCurve(type);
    switch (type) {
        case EasingType::EaseIn:       m_cp1 = {0.42f, 0}; m_cp2 = {1, 1}; break;
        case EasingType::EaseOut:      m_cp1 = {0, 0}; m_cp2 = {0.58f, 1}; break;
        case EasingType::EaseInOut:    m_cp1 = {0.42f, 0}; m_cp2 = {0.58f, 1}; break;
        default: break;
    }
    if (on_curve_changed) on_curve_changed(type);
}

Point CurveEditor::canvas_to_normalized(Point p, Rect area) const {
    float cx = area.x + m_pad;
    float cy = area.y + m_pad;
    float cw = area.width - m_pad * 2;
    float ch = area.height - m_pad * 2;
    return {std::clamp((p.x - cx) / cw, 0.0f, 1.0f), std::clamp(1.0f - (p.y - cy) / ch, 0.0f, 1.0f)};
}

Point CurveEditor::normalized_to_canvas(Point p, Rect area) const {
    float cx = area.x + m_pad;
    float cy = area.y + m_pad;
    float cw = area.width - m_pad * 2;
    float ch = area.height - m_pad * 2;
    return {cx + p.x * cw, cy + (1.0f - p.y) * ch};
}

void CurveEditor::draw_background(PaintContext& ctx, Rect area) const {
    ctx.draw_rect(area, Color{0.14f, 0.14f, 0.18f, 1}, 4);

    float cx = area.x + m_pad;
    float cy = area.y + m_pad;
    float cw = area.width - m_pad * 2;
    float ch = area.height - m_pad * 2;

    for (int i = 0; i <= 4; i++) {
        float t = i / 4.0f;
        float x = cx + t * cw;
        float y = cy + t * ch;
        ctx.draw_line({cx, y}, {cx + cw, y}, Color{0.2f, 0.2f, 0.24f, 0.5f}, 1);
        ctx.draw_line({x, cy}, {x, cy + ch}, Color{0.2f, 0.2f, 0.24f, 0.5f}, 1);
    }

    Point p0 = normalized_to_canvas({0, 0}, area);
    Point p1 = normalized_to_canvas({1, 1}, area);
    ctx.draw_line(p0, p1, Color{0.3f, 0.3f, 0.35f, 0.8f}, 1);
}

void CurveEditor::draw_curve(PaintContext& ctx, Rect area) const {
    Point prev = normalized_to_canvas({0, 0}, area);
    for (int i = 1; i <= 64; i++) {
        float t = i / 64.0f;
        float v = m_curve.evaluate(t);
        Point pt = normalized_to_canvas({t, v}, area);
        ctx.draw_line(prev, pt, Color{0.31f, 0.76f, 0.97f, 1}, 2);
        prev = pt;
    }
}

void CurveEditor::draw_control_points(PaintContext& ctx, Rect area) const {
    Point cp1c = normalized_to_canvas(m_cp1, area);
    Point cp2c = normalized_to_canvas(m_cp2, area);
    Point p0c = normalized_to_canvas({0, 0}, area);
    Point p1c = normalized_to_canvas({1, 1}, area);

    ctx.draw_line(p0c, cp1c, Color{0.5f, 0.5f, 0.6f, 0.5f}, 1);
    ctx.draw_line(p1c, cp2c, Color{0.5f, 0.5f, 0.6f, 0.5f}, 1);

    ctx.draw_circle(cp1c, m_cp_radius, Color{0.31f, 0.76f, 0.97f, 1});
    ctx.draw_circle(cp2c, m_cp_radius, Color{0.31f, 0.76f, 0.97f, 1});
    ctx.draw_border({cp1c.x - m_cp_radius, cp1c.y - m_cp_radius, m_cp_radius * 2, m_cp_radius * 2},
                    Color{1, 1, 1, 0.8f}, 1, m_cp_radius);
    ctx.draw_border({cp2c.x - m_cp_radius, cp2c.y - m_cp_radius, m_cp_radius * 2, m_cp_radius * 2},
                    Color{1, 1, 1, 0.8f}, 1, m_cp_radius);
}

void CurveEditor::on_paint(PaintContext& ctx) {
    Rect f = frame();
    draw_background(ctx, f);
    draw_curve(ctx, f);
    draw_control_points(ctx, f);
}

bool CurveEditor::on_event(InputEvent& ev) {
    Rect f = frame();
    if (!f.contains(ev.pos) && m_drag_cp < 0) return false;

    if (ev.type == EventType::MouseDown) {
        Point np = canvas_to_normalized(ev.pos, f);
        float d1 = std::hypot(np.x - m_cp1.x, np.y - m_cp1.y);
        float d2 = std::hypot(np.x - m_cp2.x, np.y - m_cp2.y);
        float threshold = m_cp_radius * 3.0f / std::min(f.width, f.height);

        if (d1 < threshold) { m_drag_cp = 1; return true; }
        if (d2 < threshold) { m_drag_cp = 2; return true; }
    }

    if (ev.type == EventType::MouseMove && m_drag_cp > 0) {
        Point np = canvas_to_normalized(ev.pos, f);
        if (m_drag_cp == 1) m_cp1 = np;
        if (m_drag_cp == 2) m_cp2 = np;
        return true;
    }

    if (ev.type == EventType::MouseUp) {
        m_drag_cp = -1;
        return true;
    }

    return false;
}

} // namespace ovui
