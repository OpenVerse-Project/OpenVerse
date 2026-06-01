#include "DesignSurface.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <climits>

namespace ovui {

DesignSurface::DesignSurface() {
    set_widget_type("DesignSurface");
    layout_node().direction = FlexDirection::Column;
    m_show_grid = true;
    m_show_handles = true;
    m_show_guides = true;
    m_grid_enabled = true;
    m_snap_enabled = true;
}

Point DesignSurface::canvas_to_surface(Point p) const {
    Rect f = frame();
    float cx = f.x + f.width * 0.5f + m_pan_offset.x;
    float cy = f.y + f.height * 0.5f + m_pan_offset.y;
    return {(p.x - cx) / m_zoom, (p.y - cy) / m_zoom};
}

Point DesignSurface::surface_to_canvas(Point p) const {
    Rect f = frame();
    float cx = f.x + f.width * 0.5f + m_pan_offset.x;
    float cy = f.y + f.height * 0.5f + m_pan_offset.y;
    return {cx + p.x * m_zoom, cy + p.y * m_zoom};
}

void DesignSurface::select(WidgetPtr widget) {
    m_selection.clear();
    if (widget) {
        m_selection.push_back(widget);
    }
    if (on_selection_changed) {
        on_selection_changed(widget);
    }
}

void DesignSurface::add_to_selection(WidgetPtr widget) {
    if (!widget) return;
    if (!is_selected(widget)) {
        m_selection.push_back(widget);
        if (on_selection_changed) on_selection_changed(widget);
    }
}

void DesignSurface::remove_from_selection(WidgetPtr widget) {
    auto it = std::find(m_selection.begin(), m_selection.end(), widget);
    if (it != m_selection.end()) {
        m_selection.erase(it);
        if (on_selection_changed) on_selection_changed(selected());
    }
}

void DesignSurface::deselect_all() {
    m_selection.clear();
    if (on_selection_changed) on_selection_changed(nullptr);
}

WidgetPtr DesignSurface::selected() const {
    return m_selection.empty() ? nullptr : m_selection.back();
}

bool DesignSurface::is_selected(WidgetPtr widget) const {
    return std::find(m_selection.begin(), m_selection.end(), widget) != m_selection.end();
}

void DesignSurface::register_widget_type(const std::string& type, WidgetFactory factory) {
    m_factories[type] = std::move(factory);
}

bool DesignSurface::has_widget_type(const std::string& type) const {
    return m_factories.find(type) != m_factories.end();
}

WidgetPtr DesignSurface::create_widget(const std::string& type) const {
    auto it = m_factories.find(type);
    if (it != m_factories.end()) {
        auto w = it->second();
        if (on_widget_created) on_widget_created(w, type);
        return w;
    }
    return nullptr;
}

HandlePart DesignSurface::hit_test_handle(const Rect& frame, Point pos, float handle_size) const {
    float hs = handle_size * 0.5f;
    float x = frame.x, y = frame.y, w = frame.width, h = frame.height;

    if (pos.x < x - hs || pos.x > x + w + hs || pos.y < y - hs || pos.y > y + h + hs) {
        return HandlePart::None;
    }

    if (pos.x >= x + w * 0.25f && pos.x <= x + w * 0.75f && pos.y >= y - hs && pos.y <= y + hs) {
        return HandlePart::Top;
    }
    if (pos.x >= x + w * 0.25f && pos.x <= x + w * 0.75f && pos.y >= y + h - hs && pos.y <= y + h + hs) {
        return HandlePart::Bottom;
    }
    if (pos.y >= y + h * 0.25f && pos.y <= y + h * 0.75f && pos.x >= x - hs && pos.x <= x + hs) {
        return HandlePart::Left;
    }
    if (pos.y >= y + h * 0.25f && pos.y <= y + h * 0.75f && pos.x >= x + w - hs && pos.x <= x + w + hs) {
        return HandlePart::Right;
    }

    if (pos.x >= x - hs && pos.x <= x + hs && pos.y >= y - hs && pos.y <= y + hs) {
        return HandlePart::TopLeft;
    }
    if (pos.x >= x + w - hs && pos.x <= x + w + hs && pos.y >= y - hs && pos.y <= y + hs) {
        return HandlePart::TopRight;
    }
    if (pos.x >= x - hs && pos.x <= x + hs && pos.y >= y + h - hs && pos.y <= y + h + hs) {
        return HandlePart::BottomLeft;
    }
    if (pos.x >= x + w - hs && pos.x <= x + w + hs && pos.y >= y + h - hs && pos.y <= y + h + hs) {
        return HandlePart::BottomRight;
    }

    return HandlePart::None;
}

Point DesignSurface::snap_to_grid(Point p, bool force) const {
    if (!m_snap_enabled && !force) return p;
    float gs = m_grid_size;
    return {std::round(p.x / gs) * gs, std::round(p.y / gs) * gs};
}

void DesignSurface::compute_alignment_guides(const Rect& target, WidgetPtr exclude) {
    m_cached_guides.clear();
    if (!m_show_guides) return;

    float tx = target.x, ty = target.y;
    float tr = target.right(), tb = target.bottom();
    float tc = target.x + target.width * 0.5f;
    float tmid = target.y + target.height * 0.5f;

    for (auto& child : children()) {
        if (child == exclude || !child->is_visible()) continue;
        Rect f = child->frame();
        float x = f.x, y = f.y;
        float r = f.right(), b = f.bottom();
        float c = f.x + f.width * 0.5f;
        float mid = f.y + f.height * 0.5f;

        if (std::abs(tx - x) < k_guide_threshold)      m_cached_guides.push_back({x, true});
        if (std::abs(tr - r) < k_guide_threshold)      m_cached_guides.push_back({r, true});
        if (std::abs(tc - c) < k_guide_threshold)      m_cached_guides.push_back({c, true});
        if (std::abs(ty - y) < k_guide_threshold)      m_cached_guides.push_back({y, false});
        if (std::abs(tb - b) < k_guide_threshold)      m_cached_guides.push_back({b, false});
        if (std::abs(tmid - mid) < k_guide_threshold)  m_cached_guides.push_back({mid, false});
    }
}

WidgetPtr DesignSurface::hit_test_widget(Point p) const {
    for (auto it = children().rbegin(); it != children().rend(); ++it) {
        if ((*it)->is_visible() && (*it)->frame().contains(p)) {
            return *it;
        }
    }
    return nullptr;
}

void DesignSurface::draw_grid(PaintContext& ctx) const {
    if (!m_show_grid || m_grid_size <= 0) return;
    Rect f = frame();
    float gs = m_grid_size * m_zoom;
    if (gs < 4) return;

    float cx = f.x + f.width * 0.5f + m_pan_offset.x * m_zoom;
    float cy = f.y + f.height * 0.5f + m_pan_offset.y * m_zoom;

    float start_x = fmod(cx - f.x, gs);
    float start_y = fmod(cy - f.y, gs);

    Color grid_color{0.5f, 0.5f, 0.5f, 0.15f};

    for (float x = f.x + start_x; x <= f.right(); x += gs) {
        ctx.draw_line({x, f.y}, {x, f.bottom()}, grid_color, 1);
    }
    for (float y = f.y + start_y; y <= f.bottom(); y += gs) {
        ctx.draw_line({f.x, y}, {f.right(), y}, grid_color, 1);
    }
}

void DesignSurface::draw_selection_handles(PaintContext& ctx, const Rect& frame) const {
    if (!m_show_handles) return;

    float hs = k_handle_size;
    float hs2 = hs * 0.5f;
    float x = frame.x, y = frame.y, w = frame.width, h = frame.height;

    Color handle_bg{1, 1, 1, 1};
    Color handle_border{0.25f, 0.5f, 0.9f, 1};
    Color sel_border = handle_border;

    ctx.draw_border(frame, sel_border, 1, 0);

    struct HandlePos { float px, py; };
    HandlePos handles[] = {
        {x - hs2, y - hs2}, {x + w * 0.5f - hs2, y - hs2}, {x + w - hs2, y - hs2},
        {x + w - hs2, y + h * 0.5f - hs2},
        {x + w - hs2, y + h - hs2}, {x + w * 0.5f - hs2, y + h - hs2}, {x - hs2, y + h - hs2},
        {x - hs2, y + h * 0.5f - hs2},
    };

    for (auto& hp : handles) {
        ctx.draw_rect({hp.px, hp.py, hs, hs}, handle_bg, 1);
        ctx.draw_border({hp.px, hp.py, hs, hs}, handle_border, 1, 1);
    }
}

void DesignSurface::draw_alignment_guides(PaintContext& ctx) const {
    if (!m_show_guides) return;
    Rect f = frame();
    Color guide_color{0.3f, 0.7f, 1.0f, 0.5f};

    for (auto& g : m_cached_guides) {
        if (g.is_vertical) {
            ctx.draw_line({g.position, f.y}, {g.position, f.bottom()}, guide_color, 1);
        } else {
            ctx.draw_line({f.x, g.position}, {f.right(), g.position}, guide_color, 1);
        }
    }
}

void DesignSurface::draw_overlay(PaintContext& ctx) const {
    if (m_drag_mode != DragMode::BoxSelect) return;
    Color overlay_bg{0.3f, 0.6f, 1.0f, 0.1f};
    Color overlay_border{0.3f, 0.6f, 1.0f, 0.4f};
    ctx.draw_rect(m_overlay_frame, overlay_bg, 0);
    ctx.draw_border(m_overlay_frame, overlay_border, 1, 0);
}

void DesignSurface::draw_surface_background(PaintContext& ctx) const {
    Color bg{0.15f, 0.15f, 0.18f, 1};
    ctx.draw_rect(frame(), bg, 0);
}

void DesignSurface::on_paint(PaintContext& ctx) {
    draw_surface_background(ctx);
    draw_grid(ctx);

    for (auto& child : children()) {
        child->on_paint(ctx);
    }

    draw_alignment_guides(ctx);

    for (auto& sel : m_selection) {
        if (sel && sel->is_visible()) {
            draw_selection_handles(ctx, sel->frame());
        }
    }

    draw_overlay(ctx);
}

void DesignSurface::align_selected(AlignEdge edge) {
    if (m_selection.empty()) return;
    auto w = selected();
    if (!w) return;
    Rect f = w->frame();

    float left = f.x, right = f.right(), top = f.y, bottom = f.bottom();
    float center_h = f.x + f.width * 0.5f, center_v = f.y + f.height * 0.5f;

    for (auto& child : children()) {
        if (child == w || !child->is_visible()) continue;
        Rect cf = child->frame();
        if (edge == AlignEdge::Left)       left = std::min(left, cf.x);
        if (edge == AlignEdge::Right)      right = std::max(right, cf.right());
        if (edge == AlignEdge::Top)        top = std::min(top, cf.y);
        if (edge == AlignEdge::Bottom)     bottom = std::max(bottom, cf.bottom());
        if (edge == AlignEdge::CenterH)    center_h = std::min(center_h, cf.x + cf.width * 0.5f);
        if (edge == AlignEdge::CenterV)    center_v = std::min(center_v, cf.y + cf.height * 0.5f);
    }

    for (auto& child : children()) {
        if (child == w || !child->is_visible()) continue;
        Rect cf = child->frame();
        if (edge == AlignEdge::Left)       left = std::min(left, cf.x);
        if (edge == AlignEdge::Right)      right = std::max(right, cf.right());
        if (edge == AlignEdge::Top)        top = std::min(top, cf.y);
        if (edge == AlignEdge::Bottom)     bottom = std::max(bottom, cf.bottom());
        if (edge == AlignEdge::CenterH)    center_h = (center_h + cf.x + cf.width * 0.5f) * 0.5f;
        if (edge == AlignEdge::CenterV)    center_v = (center_v + cf.y + cf.height * 0.5f) * 0.5f;
    }

    Rect new_frame = f;
    switch (edge) {
        case AlignEdge::Left:     new_frame.x = left; break;
        case AlignEdge::Right:    new_frame.x = right - f.width; break;
        case AlignEdge::Top:      new_frame.y = top; break;
        case AlignEdge::Bottom:   new_frame.y = bottom - f.height; break;
        case AlignEdge::CenterH:  new_frame.x = center_h - f.width * 0.5f; break;
        case AlignEdge::CenterV:  new_frame.y = center_v - f.height * 0.5f; break;
    }

    w->set_frame(new_frame);
    if (on_widget_moved) on_widget_moved(w, new_frame);
}

void DesignSurface::distribute_horizontal() {
    if (m_selection.size() < 3) return;
    std::vector<WidgetPtr> sorted = m_selection;
    std::sort(sorted.begin(), sorted.end(), [](const WidgetPtr& a, const WidgetPtr& b) {
        return a->frame().x < b->frame().x;
    });
    float left = sorted.front()->frame().x;
    float right = sorted.back()->frame().right();
    float total_width = 0;
    for (auto& w : sorted) total_width += w->frame().width;
    float spacing = (right - left - total_width) / (float)(sorted.size() - 1);
    float x = left;
    for (auto& w : sorted) {
        Rect f = w->frame();
        f.x = x;
        w->set_frame(f);
        x += f.width + spacing;
    }
}

void DesignSurface::distribute_vertical() {
    if (m_selection.size() < 3) return;
    std::vector<WidgetPtr> sorted = m_selection;
    std::sort(sorted.begin(), sorted.end(), [](const WidgetPtr& a, const WidgetPtr& b) {
        return a->frame().y < b->frame().y;
    });
    float top = sorted.front()->frame().y;
    float bottom = sorted.back()->frame().bottom();
    float total_height = 0;
    for (auto& w : sorted) total_height += w->frame().height;
    float spacing = (bottom - top - total_height) / (float)(sorted.size() - 1);
    float y = top;
    for (auto& w : sorted) {
        Rect f = w->frame();
        f.y = y;
        w->set_frame(f);
        y += f.height + spacing;
    }
}

void DesignSurface::bring_to_front() {
    if (!selected()) return;
    auto w = selected();
    remove_child(w);
    m_children.push_back(w);
    m_layout_node.children.push_back(&w->layout_node());
}

void DesignSurface::send_to_back() {
    if (!selected()) return;
    auto w = selected();
    remove_child(w);
    m_children.insert(m_children.begin(), w);
    m_layout_node.children.insert(m_layout_node.children.begin(), &w->layout_node());
}

void DesignSurface::nudge(float dx, float dy) {
    if (!selected()) return;
    Rect f = selected()->frame();
    f.x += dx;
    f.y += dy;
    selected()->set_frame(f);
}

bool DesignSurface::on_event(InputEvent& ev) {
    if (ev.type == EventType::MouseDown) {
        Point surface_pt = ev.pos;

        if (ev.modifiers & 2) {
            auto widget = hit_test_widget(surface_pt);
            if (widget) {
                if (is_selected(widget)) {
                    remove_from_selection(widget);
                } else {
                    add_to_selection(widget);
                }
            }
            return true;
        }

        if (m_selection.size() == 1 && m_show_handles) {
            auto sel = selected();
            auto hp = hit_test_handle(sel->frame(), surface_pt, k_handle_size);
            if (hp != HandlePart::None) {
                m_drag_mode = DragMode::Resize;
                m_active_handle = hp;
                m_drag_start = surface_pt;
                m_drag_original_frame = sel->frame();
                m_active_widget_original_pos = {sel->frame().x, sel->frame().y};
                return true;
            }
        }

        auto widget = hit_test_widget(surface_pt);
        if (widget && !is_selected(widget)) {
            select(widget);
        }

        if (widget) {
            m_drag_mode = DragMode::Move;
            m_drag_start = surface_pt;
            m_drag_original_frame = widget->frame();
            m_active_widget_original_pos = {widget->frame().x, widget->frame().y};
            compute_alignment_guides(widget->frame(), widget);
            return true;
        } else {
            deselect_all();
            m_drag_mode = DragMode::BoxSelect;
            m_drag_start = surface_pt;
            m_overlay_frame = {surface_pt.x, surface_pt.y, 0, 0};
            return true;
        }
    }

    if (ev.type == EventType::MouseMove) {
        if (m_drag_mode == DragMode::Move && selected()) {
            auto w = selected();
            Point delta = {ev.pos.x - m_drag_start.x, ev.pos.y - m_drag_start.y};
            Rect f = m_drag_original_frame;
            f.x += delta.x;
            f.y += delta.y;

            if (m_snap_enabled) {
                Point snapped = snap_to_grid({f.x, f.y});
                f.x = snapped.x;
                f.y = snapped.y;
            }

            compute_alignment_guides(f, w);
            w->set_frame(f);
            return true;
        }
        if (m_drag_mode == DragMode::Resize && selected()) {
            auto w = selected();
            Point delta = {ev.pos.x - m_drag_start.x, ev.pos.y - m_drag_start.y};
            Rect f = m_drag_original_frame;

            auto clamp_size = [&](float& v, float min_v) { v = std::max(min_v, v); };

            switch (m_active_handle) {
                case HandlePart::TopLeft:
                    f.x += delta.x; f.y += delta.y;
                    f.width -= delta.x; f.height -= delta.y;
                    break;
                case HandlePart::Top:
                    f.y += delta.y; f.height -= delta.y;
                    break;
                case HandlePart::TopRight:
                    f.y += delta.y; f.height -= delta.y;
                    f.width += delta.x;
                    break;
                case HandlePart::Right:
                    f.width += delta.x;
                    break;
                case HandlePart::BottomRight:
                    f.width += delta.x; f.height += delta.y;
                    break;
                case HandlePart::Bottom:
                    f.height += delta.y;
                    break;
                case HandlePart::BottomLeft:
                    f.x += delta.x; f.width -= delta.x;
                    f.height += delta.y;
                    break;
                case HandlePart::Left:
                    f.x += delta.x; f.width -= delta.x;
                    break;
                default: break;
            }
            clamp_size(f.width, m_min_size.width);
            clamp_size(f.height, m_min_size.height);

            if (m_snap_enabled) {
                Point snapped = snap_to_grid({f.x, f.y});
                f.x = snapped.x;
                f.y = snapped.y;
            }

            w->set_frame(f);
            return true;
        }
        if (m_drag_mode == DragMode::BoxSelect) {
            float x1 = std::min(m_drag_start.x, ev.pos.x);
            float y1 = std::min(m_drag_start.y, ev.pos.y);
            float x2 = std::max(m_drag_start.x, ev.pos.x);
            float y2 = std::max(m_drag_start.y, ev.pos.y);
            m_overlay_frame = {x1, y1, x2 - x1, y2 - y1};
            return true;
        }
    }

    if (ev.type == EventType::MouseUp) {
        if (m_drag_mode == DragMode::Move && selected()) {
            Rect f = selected()->frame();
            if (m_snap_enabled) {
                Point snapped = snap_to_grid({f.x, f.y});
                Rect snap_f = f;
                snap_f.x = snapped.x;
                snap_f.y = snapped.y;
                selected()->set_frame(snap_f);
            }
            if (on_widget_moved) on_widget_moved(selected(), selected()->frame());
            m_cached_guides.clear();
        }
        if (m_drag_mode == DragMode::Resize && selected()) {
            if (on_widget_moved) on_widget_moved(selected(), selected()->frame());
        }
        if (m_drag_mode == DragMode::BoxSelect) {
            deselect_all();
            for (auto& child : children()) {
                if (child->is_visible() && m_overlay_frame.intersect(child->frame()).width > 0) {
                    add_to_selection(child);
                }
            }
            if (m_selection.empty()) {
                deselect_all();
            }
        }
        m_drag_mode = DragMode::None;
        m_active_handle = HandlePart::None;
        return true;
    }

    if (ev.type == EventType::KeyDown) {
        if (ev.key == 27) {
            deselect_all();
            return true;
        }
        if (ev.key == 127 || ev.key == 8) {
            if (selected()) {
                remove_child(selected());
                deselect_all();
                return true;
            }
        }
        float nudge_step = m_grid_enabled ? m_grid_size : 1;
        if (ev.key == 265) { nudge(0, -nudge_step); return true; }
        if (ev.key == 264) { nudge(0, nudge_step); return true; }
        if (ev.key == 263) { nudge(-nudge_step, 0); return true; }
        if (ev.key == 262) { nudge(nudge_step, 0); return true; }
    }

    return false;
}

} // namespace ovui
