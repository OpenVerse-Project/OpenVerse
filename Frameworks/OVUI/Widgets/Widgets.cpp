#include "Widgets.h"
#include <algorithm>
#include <cstring>

namespace ovui {

Widget::Widget() {
    m_layout_node.id = (uint64_t)(uintptr_t)this;
    m_style_engine.set_default_theme();
}

void Widget::add_child(WidgetPtr child) {
    m_children.push_back(child);
    child->m_parent = shared_from_this();
    m_layout_node.children.push_back(&child->m_layout_node);
    child->m_layout_node.parent = &m_layout_node;
}

void Widget::remove_child(WidgetPtr child) {
    auto it = std::find(m_children.begin(), m_children.end(), child);
    if (it != m_children.end()) {
        auto lit = std::find(m_layout_node.children.begin(), m_layout_node.children.end(),
                             &child->m_layout_node);
        if (lit != m_layout_node.children.end()) m_layout_node.children.erase(lit);
        child->m_parent.reset();
        child->m_layout_node.parent = nullptr;
        m_children.erase(it);
    }
}

void Widget::remove_all_children() {
    for (auto& c : m_children) {
        c->m_parent.reset();
        c->m_layout_node.parent = nullptr;
    }
    m_children.clear();
    m_layout_node.children.clear();
}

void Widget::set_state(WidgetState s) {
    WidgetState old = m_state;
    m_state = s;
    if (old != s) {
        m_style_dirty = true;
        on_state_changed(old, s);
    }
}

void Widget::set_enabled(bool e) {
    if (e) set_state(m_state & ~WidgetState::Disabled);
    else set_state(m_state | WidgetState::Disabled);
}

void Widget::update_layout_node() {
    m_layout_node.id = m_id;
    auto style = resolve_styles();
    auto& se = m_style_engine;
    float min_w = se.resolve_float(StylePropertyID::MinWidth, style, 0);
    float max_w = se.resolve_float(StylePropertyID::MaxWidth, style, INFINITY);
    float min_h = se.resolve_float(StylePropertyID::MinHeight, style, 0);
    float max_h = se.resolve_float(StylePropertyID::MaxHeight, style, INFINITY);
    float w = se.resolve_float(StylePropertyID::Width, style, -1);
    float h = se.resolve_float(StylePropertyID::Height, style, -1);

    m_layout_node.constraints.min_width = min_w;
    m_layout_node.constraints.max_width = max_w;
    m_layout_node.constraints.min_height = min_h;
    m_layout_node.constraints.max_height = max_h;
    if (w > 0) m_layout_node.constraints.flex_basis = w;
    if (h > 0) m_layout_node.result.frame.height = h;
    m_layout_node.padding = se.resolve_edge_insets(style);
    m_layout_node.gap = se.resolve_float(StylePropertyID::Gap, style, 0);
    m_layout_node.cross_gap = se.resolve_float(StylePropertyID::CrossGap, style, 0);
}

std::vector<StyleValue> Widget::resolve_styles() const {
    if (!m_style_dirty && !m_cached_style.empty()) return m_cached_style;
    m_cached_style = m_style_engine.resolve(m_widget_type, m_class_name, m_state);
    m_style_dirty = false;
    return m_cached_style;
}

std::vector<StyleValue> Widget::resolved_style() const {
    return resolve_styles();
}

void PaintContext::draw_rect(const Rect& rect, Color color, float radius) {
    m_cmds.push_back({DrawCmd::RectCmd, rect, color, "", radius});
}

void PaintContext::draw_text(const std::string& text, Point pos, Color color, float size) {
    m_cmds.push_back({DrawCmd::TextCmd, {pos.x, pos.y, 0, 0}, color, text, size});
}

void PaintContext::draw_border(const Rect& rect, Color color, float width, float radius) {
    m_cmds.push_back({DrawCmd::BorderCmd, rect, color, "", radius, width});
}

void PaintContext::draw_circle(Point center, float radius, Color c) {
    m_cmds.push_back({DrawCmd::RectCmd, {center.x-radius, center.y-radius, radius*2, radius*2}, c, "", radius});
}

void PaintContext::draw_line(Point p1, Point p2, Color c, float w) {
    float l = std::min(p1.x, p2.x);
    float t = std::min(p1.y, p2.y);
    float r = std::max(p1.x, p2.x);
    float b = std::max(p1.y, p2.y);
    float vw = (r - l < w) ? w : 0;
    float vh = (b - t < w) ? w : 0;
    m_cmds.push_back({DrawCmd::LineCmd, {l, t, std::max(vw, r - l), std::max(vh, b - t)}, c, "", 0, w});
}

// ---- Container ----
Container::Container() { set_widget_type("Box"); }

// ---- Box ----
Box::Box() { set_widget_type("Box"); }
void Box::on_paint(PaintContext& ctx) {
    auto style = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, style,
                                             Color::from_hex(0x1E1E2EFF));
    float radius = m_style_engine.resolve_float(StylePropertyID::BorderRadius, style, 0);
    ctx.draw_rect(frame(), bg, radius);
}

// ---- Text ----
Text::Text() { set_widget_type("Text"); }
void Text::on_paint(PaintContext& ctx) {
    auto style = resolve_styles();
    Color c = m_style_engine.resolve_color(StylePropertyID::Color, style,
                                            Color::from_hex(0xCDD6F4FF));
    float size = m_style_engine.resolve_float(StylePropertyID::FontSize, style, 13);
    ctx.draw_text(m_text, {frame().x, frame().y}, c, size);
}

// ---- Button ----
Button::Button() { set_widget_type("Button"); }
void Button::on_paint(PaintContext& ctx) {
    auto style = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, style,
                                             Color::from_hex(0x4FC3F7FF));
    float radius = m_style_engine.resolve_float(StylePropertyID::BorderRadius, style, 6);
    ctx.draw_rect(frame(), bg, radius);

    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, style, Color{1,1,1,1});
    float fontSize = m_style_engine.resolve_float(StylePropertyID::FontSize, style, 14);
    Rect f = frame();
    float tw = m_label.size() * fontSize * 0.6f;
    ctx.draw_text(m_label, {f.center().x - tw*0.5f, f.center().y - fontSize*0.5f}, fg, fontSize);
}
bool Button::on_event(InputEvent& ev) {
    if (!is_enabled() || !is_visible()) return false;
    if (ev.type == EventType::MouseDown && ev.button == 0 && frame().contains(ev.pos)) {
        set_state(m_state | WidgetState::Pressed);
        return true;
    }
    if (ev.type == EventType::MouseUp && ev.button == 0 && has_state(WidgetState::Pressed)) {
        set_state(m_state & ~WidgetState::Pressed);
        if (frame().contains(ev.pos)) {
            m_clicks.set(m_clicks.get() + 1);
            if (on_click) on_click();
        }
        return true;
    }
    if (ev.type == EventType::MouseMove) {
        if (frame().contains(ev.pos)) {
            if (!has_state(WidgetState::Hover)) set_state(m_state | WidgetState::Hover);
        } else {
            if (has_state(WidgetState::Hover)) set_state(m_state & ~WidgetState::Hover);
        }
    }
    return false;
}

// ---- Slider ----
Slider::Slider() { set_widget_type("Slider"); }
void Slider::on_paint(PaintContext& ctx) {
    auto style = resolve_styles();
    Color track_color = m_style_engine.resolve_color(StylePropertyID::BorderColor, style,
                                                      Color::from_hex(0x3C3C5AFF));
    float radius = m_style_engine.resolve_float(StylePropertyID::BorderRadius, style, 2);
    Rect f = frame();
    float ratio = (m_value.get() - m_min) / (m_max - m_min);
    float thumb_x = f.x + ratio * f.width;

    ctx.draw_rect({f.x, f.center().y - 2, f.width, 4}, track_color, 2);
    ctx.draw_rect({thumb_x - 8, f.center().y - 8, 16, 16},
                  Color::from_hex(0x4FC3F7FF), 8);
}
bool Slider::on_event(InputEvent& ev) {
    if (!is_enabled()) return false;
    if (ev.type == EventType::MouseDown && frame().contains(ev.pos)) {
        set_state(m_state | WidgetState::Pressed);
        float ratio = (ev.pos.x - frame().x) / frame().width;
        m_value.set(m_min + ratio * (m_max - m_min));
        return true;
    }
    if (ev.type == EventType::MouseMove && has_state(WidgetState::Pressed)) {
        float ratio = std::clamp((ev.pos.x - frame().x) / frame().width, 0.0f, 1.0f);
        m_value.set(m_min + ratio * (m_max - m_min));
        return true;
    }
    if (ev.type == EventType::MouseUp) {
        set_state(m_state & ~WidgetState::Pressed);
        return true;
    }
    return false;
}

// ---- TextInput ----
TextInput::TextInput() { set_widget_type("TextInput"); }
void TextInput::on_paint(PaintContext& ctx) {
    auto style = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, style,
                                             Color::from_hex(0x282848FF));
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, style,
                                             Color::from_hex(0xCDD6F4FF));
    Color border_c = m_style_engine.resolve_color(StylePropertyID::BorderColor, style,
                                                    Color::from_hex(0x3C3C5AFF));
    float radius = m_style_engine.resolve_float(StylePropertyID::BorderRadius, style, 4);
    float bw = m_style_engine.resolve_float(StylePropertyID::BorderWidth, style, 1);
    float fontSize = m_style_engine.resolve_float(StylePropertyID::FontSize, style, 13);

    ctx.draw_rect(frame(), bg, radius);
    ctx.draw_border(frame(), border_c, bw, radius);

    auto& txt = m_text.get();
    if (!txt.empty()) {
        ctx.draw_text(txt, {frame().x + 10, frame().y + 6}, fg, fontSize);
    } else if (!m_placeholder.empty()) {
        Color ph = fg; ph.a = 0.4f;
        ctx.draw_text(m_placeholder, {frame().x + 10, frame().y + 6}, ph, fontSize);
    }
}
bool TextInput::on_event(InputEvent& ev) {
    if (!is_enabled()) return false;
    if (ev.type == EventType::MouseDown && frame().contains(ev.pos)) {
        set_state(m_state | WidgetState::Focused);
        return true;
    }
    if (ev.type == EventType::MouseDown && has_state(WidgetState::Focused) && !frame().contains(ev.pos)) {
        set_state(m_state & ~WidgetState::Focused);
    }
    if (ev.type == EventType::KeyDown && has_state(WidgetState::Focused)) {
        if (ev.key == 259) {
            auto t = m_text.get();
            if (!t.empty()) { t.pop_back(); m_text.set(t); }
            return true;
        }
        return true;
    }
    if (ev.type == EventType::TextInput && has_state(WidgetState::Focused)) {
        m_text.set(m_text.get() + ev.text);
        return true;
    }
    return false;
}
void TextInput::set_text(const std::string& t) { m_text.set(t); }

// ---- ScrollArea ----
ScrollArea::ScrollArea() {
    set_widget_type("ScrollArea");
    set_direction(FlexDirection::Column);
}
void ScrollArea::on_paint(PaintContext& ctx) {
}
bool ScrollArea::on_event(InputEvent& ev) {
    if (ev.type == EventType::MouseWheel && frame().contains(ev.pos)) {
        set_scroll_y(m_scroll_y + ev.wheel_delta * 20);
        return true;
    }
    return Widget::on_event(ev);
}
void ScrollArea::set_scroll_y(float y) {
    m_scroll_y = std::max(0.0f, y);
}

} // namespace ovui
