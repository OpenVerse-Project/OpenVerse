#include "Advanced.h"
#include <algorithm>
#include <cmath>
#include <cstring>

namespace ovui {

// ============================================================
// RadioGroup — fixed with enable_shared_from_this
// ============================================================
void RadioGroup::add(std::shared_ptr<RadioButton> btn) {
    m_buttons.push_back(btn);
    btn->set_group(shared_from_this());
}
void RadioGroup::select(std::shared_ptr<RadioButton> btn) {
    for (auto& w : m_buttons) {
        if (auto b = w.lock()) b->set_selected(b == btn);
    }
    m_selected = btn;
}

// ============================================================
// Checkbox — unicode checkmark
// ============================================================
Checkbox::Checkbox() { set_widget_type("Checkbox"); }
void Checkbox::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x282848FF));
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color accent = Color::from_hex(0x4FC3F7FF);
    Rect f = frame();
    float cb = 16;
    ctx.draw_rect({f.x, f.y + (f.height - cb) * 0.5f, cb, cb}, m_checked.get() ? accent : bg, 3);
    if (m_checked.get()) {
        ctx.draw_text("\xe2\x9c\x93", {f.x + 3, f.y + 2}, Color{1,1,1,1}, 11);
    }
    ctx.draw_text(m_label, {f.x + cb + 8, f.y + 2}, fg, 13);
}
bool Checkbox::on_event(InputEvent& ev) {
    if (!is_enabled()) return false;
    if (ev.type == EventType::MouseDown && frame().contains(ev.pos)) {
        m_checked.set(!m_checked.get());
        if (on_toggle) on_toggle(m_checked.get());
        return true;
    }
    return false;
}

// ============================================================
// RadioButton — circle rendering
// ============================================================
RadioButton::RadioButton() { set_widget_type("RadioButton"); }
void RadioButton::set_group(std::shared_ptr<RadioGroup> g) { m_group = g; }
void RadioButton::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color accent = Color::from_hex(0x4FC3F7FF);
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x282848FF));
    Rect f = frame();
    float outer_r = 8, inner_r = 4;
    Point center{f.x + outer_r + 2, f.y + f.height * 0.5f};
    ctx.draw_circle(center, outer_r, m_selected.get() ? accent : bg);
    if (m_selected.get()) {
        ctx.draw_circle(center, inner_r, Color{1,1,1,1});
    }
    ctx.draw_text(m_label, {f.x + outer_r * 2 + 10, f.y + 2}, fg, 13);
}
bool RadioButton::on_event(InputEvent& ev) {
    if (!is_enabled()) return false;
    if (ev.type == EventType::MouseDown && frame().contains(ev.pos)) {
        if (auto g = m_group.lock()) g->select(std::static_pointer_cast<RadioButton>(shared_from_this()));
        else m_selected.set(true);
        if (on_select) on_select();
        return true;
    }
    return false;
}

// ============================================================
// ComboBox — full dropdown rendering + selection
// ============================================================
ComboBox::ComboBox() { set_widget_type("ComboBox"); }
void ComboBox::set_items(const std::vector<std::string>& items) {
    m_items = items;
    m_open = false;
    m_popup_height = std::min((int)items.size(), 10) * 24.0f + 4;
}
void ComboBox::set_selected_index(int idx) {
    if (idx >= -1 && idx < (int)m_items.size()) { m_selected_idx.set(idx); m_open = false; }
    if (on_change) on_change(idx);
}
void ComboBox::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x282848FF));
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color border_c = m_style_engine.resolve_color(StylePropertyID::BorderColor, s, Color::from_hex(0x3C3C5AFF));
    Color accent = Color::from_hex(0x4FC3F7FF);
    float radius = m_style_engine.resolve_float(StylePropertyID::BorderRadius, s, 4);
    Rect f = frame();

    ctx.draw_rect(f, bg, radius);
    ctx.draw_border(f, border_c, 1, radius);

    std::string txt = m_selected_idx.get() >= 0 && m_selected_idx.get() < (int)m_items.size()
        ? m_items[m_selected_idx.get()] : "";
    ctx.draw_text(txt, {f.x+8, f.y+6}, fg, 13);
    ctx.draw_text("\xe2\x96\xbc", {f.right()-20, f.y+6}, fg, 10);

    if (m_open && !m_items.empty()) {
        float popup_w = f.width;
        Rect pr{f.x, f.bottom() + 2, popup_w, m_popup_height};
        ctx.draw_rect(pr, bg.darken(0.1f), radius);
        ctx.draw_border(pr, border_c, 1, radius);

        int visible = std::min((int)m_items.size(), 10);
        for (int i = 0; i < visible; i++) {
            Rect ir{pr.x + 2, pr.y + 2 + i * 24.0f, pr.width - 4, 24};
            if (i == m_selected_idx.get())
                ctx.draw_rect(ir, accent.darken(0.3f), 2);
            ctx.draw_text(m_items[i], {ir.x + 6, ir.y + 3}, fg, 13);
        }
    }
}
bool ComboBox::on_event(InputEvent& ev) {
    if (!is_enabled()) return false;
    Rect f = frame();

    if (m_open) {
        Rect pr{f.x, f.bottom() + 2, f.width, m_popup_height};
        if (ev.type == EventType::MouseDown) {
            if (pr.contains(ev.pos)) {
                int idx = (int)((ev.pos.y - pr.y) / 24.0f);
                if (idx >= 0 && idx < (int)m_items.size()) set_selected_index(idx);
                return true;
            }
            m_open = false;
            return true;
        }
    }

    if (ev.type == EventType::MouseDown && f.contains(ev.pos)) {
        m_open = !m_open;
        if (m_open) capture_mouse();
        else release_mouse();
        return true;
    }
    return false;
}

// ============================================================
// SpinBox
// ============================================================
SpinBox::SpinBox() { set_widget_type("SpinBox"); }
void SpinBox::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x282848FF));
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color border_c = m_style_engine.resolve_color(StylePropertyID::BorderColor, s, Color::from_hex(0x3C3C5AFF));
    float radius = m_style_engine.resolve_float(StylePropertyID::BorderRadius, s, 4);
    Rect f = frame();
    ctx.draw_rect(f, bg, radius);
    ctx.draw_border(f, border_c, 1, radius);
    ctx.draw_text(std::to_string(m_value.get()), {f.x+8, f.y+6}, fg, 13);
    float btn_w = 20;
    ctx.draw_text("\xe2\x96\xb2", {f.right()-btn_w-2, f.y+2}, fg, 9);
    ctx.draw_text("\xe2\x96\xbc", {f.right()-btn_w-2, f.y+14}, fg, 9);
}
bool SpinBox::on_event(InputEvent& ev) {
    if (!is_enabled()) return false;
    Rect f = frame(); float btn_w = 20;
    if (ev.type == EventType::MouseDown) {
        if (ev.pos.y < f.y + f.height * 0.5f && ev.pos.x > f.right() - btn_w) {
            set_value(m_value.get()+1); if (on_change) on_change(m_value.get()); return true;
        }
        if (ev.pos.y >= f.y + f.height * 0.5f && ev.pos.x > f.right() - btn_w) {
            set_value(m_value.get()-1); if (on_change) on_change(m_value.get()); return true;
        }
    }
    return false;
}

// ============================================================
// Window — mouse capture drag
// ============================================================
Window::Window() { set_widget_type("Window"); set_padding({30,0,0,0}); }
void Window::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x181825FF));
    Color header = Color::from_hex(0x11111BFF);
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Rect f = frame();
    ctx.draw_rect(f, bg, 8);
    ctx.draw_rect({f.x, f.y, f.width, m_titlebar_h}, header, 8);
    ctx.draw_text(m_title, {f.x+10, f.y+7}, fg, 14);
    if (m_closable)
        ctx.draw_text("\xe2\x9c\x95", {f.right()-24, f.y+5}, Color::from_hex(0xF38BA8FF), 14);
}
bool Window::on_event(InputEvent& ev) {
    Rect f = frame();
    Rect title_bar{f.x, f.y, f.width, m_titlebar_h};

    if (has_mouse_capture()) {
        if (ev.type == EventType::MouseMove) {
            set_frame({ev.pos.x - m_drag_start.x, ev.pos.y - m_drag_start.y, f.width, f.height});
            return true;
        }
        if (ev.type == EventType::MouseUp) {
            release_mouse(); m_dragging = false;
            return true;
        }
        return true;
    }

    if (ev.type == EventType::MouseDown && title_bar.contains(ev.pos)) {
        if (m_closable && ev.pos.x > f.right() - 30) {
            if (on_close) on_close();
            return true;
        }
        m_dragging = true;
        m_drag_start = {ev.pos.x - f.x, ev.pos.y - f.y};
        capture_mouse();
        return true;
    }
    return Container::on_event(ev);
}

// ============================================================
// Dialog — viewport-aware background
// ============================================================
Dialog::Dialog() { set_widget_type("Dialog"); set_modal(true); }
void Dialog::on_paint(PaintContext& ctx) {
    if (!m_showing) return;
    Size vp = ctx.viewport();
    ctx.draw_rect({0, 0, vp.width, vp.height}, Color{0,0,0,0.5f}, 0);
    Window::on_paint(ctx);
}
void Dialog::show(WidgetPtr parent, Size viewport) {
    m_showing = true;
    set_frame({viewport.width*0.2f, viewport.height*0.2f, viewport.width*0.6f, viewport.height*0.6f});
    if (parent) parent->add_child(std::static_pointer_cast<Widget>(shared_from_this()));
}
void Dialog::dismiss() {
    m_showing = false;
    if (on_dismiss) on_dismiss();
}

// ============================================================
// TabView
// ============================================================
TabView::TabView() { set_widget_type("TabView"); }
int TabView::add_tab(const std::string& label, WidgetPtr content) {
    m_tabs.push_back({label, content});
    if (m_tabs.size() == 1) set_active_tab(0);
    return (int)m_tabs.size() - 1;
}
void TabView::remove_tab(int index) {
    if (index < 0 || index >= (int)m_tabs.size()) return;
    m_tabs.erase(m_tabs.begin() + index);
    if (m_active_idx >= (int)m_tabs.size()) m_active_idx = std::max(0, (int)m_tabs.size() - 1);
}
void TabView::set_active_tab(int index) {
    remove_all_children();
    if (index >= 0 && index < (int)m_tabs.size()) {
        m_active_idx = index; add_child(m_tabs[index].content);
    }
}
void TabView::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x181825FF));
    Color tab_bg = Color::from_hex(0x11111BFF);
    Color accent = Color::from_hex(0x4FC3F7FF);
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Rect f = frame();
    ctx.draw_rect(f, bg, 0);
    float x = f.x;
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        float tw = m_tabs[i].label.size() * 7 + m_tab_padding * 2;
        ctx.draw_rect({x, f.y, tw, m_tab_height}, i == m_active_idx ? accent : tab_bg, 0);
        ctx.draw_text(m_tabs[i].label, {x + m_tab_padding, f.y + 6}, fg, 13);
        x += tw + 2;
    }
    ctx.draw_rect({f.x, f.y + m_tab_height, f.width, 2}, accent, 0);
    for (auto& c : children())
        c->set_frame({f.x, f.y + m_tab_height + 2, f.width, f.height - m_tab_height - 2});
}
bool TabView::on_event(InputEvent& ev) {
    Rect f = frame(); float x = f.x;
    for (int i = 0; i < (int)m_tabs.size(); i++) {
        float tw = m_tabs[i].label.size() * 7 + m_tab_padding * 2;
        if (ev.type == EventType::MouseDown && Rect{x, f.y, tw, m_tab_height}.contains(ev.pos)) {
            set_active_tab(i); return true;
        }
        x += tw + 2;
    }
    return Container::on_event(ev);
}

// ============================================================
// StackView — safe replace
// ============================================================
StackView::StackView() { set_widget_type("StackView"); }
void StackView::push(WidgetPtr page) {
    remove_all_children(); m_stack.push_back(page); add_child(page);
}
void StackView::pop() {
    if (m_stack.size() <= 1) return;
    m_stack.pop_back(); remove_all_children(); add_child(m_stack.back());
}
void StackView::replace(WidgetPtr page) {
    if (m_stack.empty()) { push(page); return; }
    m_stack.back() = page; remove_all_children(); add_child(page);
}

// ============================================================
// Splitter — mouse capture drag
// ============================================================
Splitter::Splitter() { set_widget_type("Splitter"); }
void Splitter::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x181825FF));
    Color splitter_c = Color::from_hex(0x31315AFF);
    Rect f = frame();
    ctx.draw_rect(f, bg, 0);
    if (m_vertical) {
        float split_y = f.y + f.height * m_ratio;
        ctx.draw_rect({f.x, split_y - m_splitter_w * 0.5f, f.width, m_splitter_w}, splitter_c, 2);
    } else {
        float split_x = f.x + f.width * m_ratio;
        ctx.draw_rect({split_x - m_splitter_w * 0.5f, f.y, m_splitter_w, f.height}, splitter_c, 2);
    }
}
bool Splitter::on_event(InputEvent& ev) {
    Rect f = frame();

    if (has_mouse_capture()) {
        if (ev.type == EventType::MouseMove) {
            m_ratio = m_vertical ? (ev.pos.y - f.y) / f.height : (ev.pos.x - f.x) / f.width;
            m_ratio = std::clamp(m_ratio, 0.1f, 0.9f);
            return true;
        }
        if (ev.type == EventType::MouseUp) { release_mouse(); m_dragging = false; return true; }
        return true;
    }

    if (ev.type == EventType::MouseDown) {
        float hit = m_vertical ?
            std::abs(ev.pos.y - (f.y + f.height * m_ratio)) :
            std::abs(ev.pos.x - (f.x + f.width * m_ratio));
        if (hit < 8) { m_dragging = true; capture_mouse(); return true; }
    }
    return Container::on_event(ev);
}

// ============================================================
// Panel
// ============================================================
Panel::Panel() { set_widget_type("Panel"); }
void Panel::set_collapsed(bool c) {
    if (c && !m_collapsed) m_expanded_height = frame().height;
    m_collapsed = c;
}
void Panel::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x181825FF));
    Color header_c = Color::from_hex(0x222240FF);
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Rect f = frame();
    ctx.draw_rect(f, bg, 6);
    ctx.draw_rect({f.x, f.y, f.width, m_header_h}, header_c, 6);
    ctx.draw_text(m_title, {f.x + 8, f.y + 4}, fg, 13);
    ctx.draw_text(m_collapsed ? "\xe2\x96\xb6" : "\xe2\x96\xbc", {f.right() - 20, f.y + 4}, fg, 10);
    if (!m_collapsed) {
        for (auto& c : children()) {
            c->set_frame({f.x + 4, f.y + m_header_h + 4, f.width - 8, f.height - m_header_h - 8});
        }
    }
}
bool Panel::on_event(InputEvent& ev) {
    Rect f = frame();
    Rect header{f.x, f.y, f.width, m_header_h};
    if (ev.type == EventType::MouseDown && header.contains(ev.pos)) {
        if (ev.pos.x > f.right() - 24) toggle();
        return true;
    }
    return Container::on_event(ev);
}

// ============================================================
// MenuBar — fixed coordinates with frame
// ============================================================
MenuBar::MenuBar() { set_widget_type("MenuBar"); }
void MenuBar::add_menu(const std::string& label, std::vector<MenuItem> items) {
    m_menus.push_back({label, std::move(items)});
}
void MenuBar::clear() { m_menus.clear(); m_open_menu = -1; }
void MenuBar::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x11111BFF));
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color accent = Color::from_hex(0x4FC3F7FF);
    Rect f = frame();
    ctx.draw_rect(f, bg, 0);
    float x = f.x;
    for (int i = 0; i < (int)m_menus.size(); i++) {
        float w = m_menus[i].label.size() * 7 + m_item_pad * 2;
        ctx.draw_rect({x, f.y, w, m_item_h}, i == m_open_menu ? accent : bg, 0);
        ctx.draw_text(m_menus[i].label, {x + m_item_pad, f.y + 5}, fg, 13);
        x += w + 2;
    }
}
bool MenuBar::on_event(InputEvent& ev) {
    Rect f = frame();
    float x = f.x;
    for (int i = 0; i < (int)m_menus.size(); i++) {
        float w = m_menus[i].label.size() * 7 + m_item_pad * 2;
        if (ev.type == EventType::MouseDown && Rect{x, f.y, w, m_item_h}.contains(ev.pos)) {
            m_open_menu = (m_open_menu == i) ? -1 : i;
            return true;
        }
        x += w + 2;
    }
    if (ev.type == EventType::MouseDown && m_open_menu >= 0) m_open_menu = -1;
    return false;
}

// ============================================================
// ContextMenu — proper hierarchy + show/dismiss
// ============================================================
ContextMenu::ContextMenu() { set_widget_type("ContextMenu"); }
void ContextMenu::set_items(std::vector<MenuItem> items) { m_items = std::move(items); }
void ContextMenu::show(Point pos, WidgetPtr owner) {
    m_showing = true; m_pos = pos; m_owner = owner;
    set_frame({pos.x, pos.y, m_min_width, m_items.size() * m_item_h + 4});
    capture_mouse();
    if (owner) {
        Rect owner_frame = owner->frame();
        if (frame().bottom() > owner_frame.bottom()) {
            set_frame({pos.x, pos.y - frame().height, frame().width, frame().height});
        }
    }
}
void ContextMenu::dismiss() { m_showing = false; release_mouse(); m_owner.reset(); }
void ContextMenu::on_paint(PaintContext& ctx) {
    if (!m_showing) return;
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x1E1E2EFF));
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color hover_c = Color::from_hex(0x4FC3F7FF);
    Rect f = frame();
    ctx.draw_rect(f, bg, 6);
    ctx.draw_border(f, Color::from_hex(0x31315AFF), 1, 6);
    for (size_t i = 0; i < m_items.size(); i++) {
        Rect ir{f.x + 2, f.y + 2 + i * m_item_h, f.width - 4, m_item_h};
        if (m_items[i].separator) {
            ctx.draw_rect({ir.x + 8, ir.y + ir.height * 0.5f, ir.width - 16, 1}, Color::from_hex(0x31315AFF), 0);
        } else {
            Color item_fg = m_items[i].enabled ? fg : Color{fg.r, fg.g, fg.b, 0.4f};
            ctx.draw_text(m_items[i].label, {ir.x + 8, ir.y + 3}, item_fg, 13);
            if (!m_items[i].shortcut.empty()) {
                float sw = m_items[i].shortcut.size() * 7;
                ctx.draw_text(m_items[i].shortcut, {ir.right() - sw - 8, ir.y + 3}, item_fg, 13);
            }
        }
    }
}
bool ContextMenu::on_event(InputEvent& ev) {
    if (!m_showing) return false;
    if (ev.type == EventType::MouseDown && !frame().contains(ev.pos)) {
        dismiss(); return true;
    }
    Rect f = frame();
    for (size_t i = 0; i < m_items.size(); i++) {
        if (m_items[i].separator) continue;
        Rect ir{f.x + 2, f.y + 2 + i * m_item_h, f.width - 4, m_item_h};
        if (ev.type == EventType::MouseDown && ir.contains(ev.pos)) {
            if (m_items[i].enabled && m_items[i].action) m_items[i].action();
            dismiss(); return true;
        }
    }
    return true;
}

// ============================================================
// ListView
// ============================================================
ListView::ListView() { set_widget_type("ListView"); }
void ListView::refresh() { m_dirty = true; }
void ListView::set_selected_index(int idx) { m_selected_idx = idx; }
void ListView::scroll_to(int index) {
    if (m_item_height > 0) { m_scroll_offset = index * m_item_height; m_dirty = true; }
}
void ListView::update_visible_items() {
    if (m_item_height <= 0) return;
    Rect f = frame();
    m_first_visible = std::max(0, (int)(m_scroll_offset / m_item_height));
    m_last_visible = std::min(m_item_count - 1, (int)((m_scroll_offset + f.height) / m_item_height) + 1);
    m_dirty = false;
}
void ListView::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x1E1E2EFF));
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color accent = Color::from_hex(0x4FC3F7FF);
    Rect f = frame();
    ctx.draw_rect(f, bg, 4);
    if (m_dirty) update_visible_items();
    for (int i = m_first_visible; i <= m_last_visible && i < m_item_count; i++) {
        float y = f.y + i * m_item_height - m_scroll_offset;
        Rect ir{f.x, y, f.width, m_item_height};
        if (i == m_selected_idx) ctx.draw_rect(ir, accent.darken(0.3f), 0);
        std::string label = "Item " + std::to_string(i);
        ctx.draw_text(label, {ir.x + 8, ir.y + 4}, fg, 13);
    }
}
bool ListView::on_event(InputEvent& ev) {
    Rect f = frame();
    if (ev.type == EventType::MouseWheel && f.contains(ev.pos)) {
        m_scroll_offset = std::max(0.0f, m_scroll_offset - ev.wheel_delta * 20); m_dirty = true; return true;
    }
    if (ev.type == EventType::MouseDown && f.contains(ev.pos)) {
        int idx = (int)((ev.pos.y - f.y + m_scroll_offset) / m_item_height);
        if (idx >= 0 && idx < m_item_count) m_selected_idx = idx;
        return true;
    }
    return Container::on_event(ev);
}

// ============================================================
// TreeView — reliable depth assignment
// ============================================================
TreeView::TreeView() { set_widget_type("TreeView"); }
void TreeView::set_root(TreeNode root) { m_root = std::move(root); assign_depths(&m_root, 0); flatten(); }
void TreeView::toggle_node(TreeNode* node) {
    if (!node || !node->has_children) return;
    node->expanded = !node->expanded; flatten();
}
void TreeView::expand_all() {
    std::function<void(TreeNode*)> rec = [&](TreeNode* n) {
        if (n->has_children) n->expanded = true;
        for (auto& c : n->children) rec(&c);
    };
    rec(&m_root); flatten();
}
void TreeView::collapse_all() {
    std::function<void(TreeNode*)> rec = [&](TreeNode* n) {
        n->expanded = false;
        for (auto& c : n->children) rec(&c);
    };
    rec(&m_root); flatten();
}
void TreeView::assign_depths(TreeNode* node, int depth) {
    node->depth = depth;
    for (auto& c : node->children) assign_depths(&c, depth + 1);
}
void TreeView::flatten() {
    m_flat.clear();
    std::function<void(TreeNode*)> rec = [&](TreeNode* n) {
        m_flat.push_back(n);
        if (n->expanded) for (auto& c : n->children) rec(&c);
    };
    for (auto& c : m_root.children) rec(&c);
}
void TreeView::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x1E1E2EFF));
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color accent = Color::from_hex(0x4FC3F7FF);
    Rect f = frame();
    ctx.draw_rect(f, bg, 4);
    for (int i = 0; i < (int)m_flat.size(); i++) {
        auto* n = m_flat[i];
        float indent = n->depth * m_indent;
        float y = f.y + i * m_row_height;
        if (n == m_selected) ctx.draw_rect({f.x, y, f.width, m_row_height}, accent.darken(0.3f), 0);
        std::string prefix = n->has_children ? (n->expanded ? "\xe2\x96\xbc " : "\xe2\x96\xb6 ") : "  ";
        ctx.draw_text(prefix + n->label, {f.x + indent + 4, y + 3}, fg, 13);
    }
}
bool TreeView::on_event(InputEvent& ev) {
    Rect f = frame();
    if (ev.type == EventType::MouseDown && f.contains(ev.pos)) {
        int idx = (int)((ev.pos.y - f.y) / m_row_height);
        if (idx >= 0 && idx < (int)m_flat.size()) {
            auto* n = m_flat[idx]; m_selected = n;
            if (n->has_children) toggle_node(n);
            if (on_select) on_select(n);
            return true;
        }
    }
    return Container::on_event(ev);
}

// ============================================================
// TableView — true horizontal virtualization + vertical
// ============================================================
TableView::TableView() { set_widget_type("TableView"); }
void TableView::set_dimensions(int rows, int cols) {
    m_rows = rows; m_cols = cols;
    m_col_widths.resize(cols, 100); m_headers.resize(cols); m_dirty = true;
}
float TableView::total_width() const {
    float w = 0; for (auto cw : m_col_widths) w += cw; return w;
}
void TableView::set_column_width(int col, float w) {
    if (col >= 0 && col < (int)m_col_widths.size()) { m_col_widths[col] = w; m_dirty = true; }
}
void TableView::set_header_labels(const std::vector<std::string>& labels) {
    m_headers = labels; m_headers.resize(m_cols);
}
void TableView::refresh() { m_dirty = true; }
void TableView::update_visible_cells() {
    Rect f = frame();
    m_first_row = std::max(0, (int)(m_scroll_y / m_row_height));
    m_last_row = std::min(m_rows - 1, (int)((m_scroll_y + f.height - m_header_height) / m_row_height) + 1);

    float x_acc = f.x - m_scroll_x;
    m_first_col = 0;
    for (int c = 0; c < m_cols; c++) {
        if (x_acc + m_col_widths[c] >= f.x) { m_first_col = c; break; }
        x_acc += m_col_widths[c];
    }
    float x_end = f.x - m_scroll_x;
    m_last_col = m_cols - 1;
    for (int c = 0; c < m_cols; c++) {
        if (x_end >= f.right()) { m_last_col = std::max(c - 1, 0); break; }
        x_end += m_col_widths[c];
    }
    m_dirty = false;
}
void TableView::on_paint(PaintContext& ctx) {
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s, Color::from_hex(0x1E1E2EFF));
    Color header_bg = Color::from_hex(0x11111BFF);
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Color accent = Color::from_hex(0x4FC3F7FF);
    Rect f = frame();
    ctx.draw_rect(f, bg, 0);
    if (m_dirty) update_visible_cells();

    float x = f.x - m_scroll_x;
    for (int c = 0; c < m_cols; c++) {
        float hx = x; x += m_col_widths[c];
        if (c < m_first_col || c > m_last_col) continue;
        Rect hr{hx, f.y, m_col_widths[c], m_header_height};
        ctx.draw_rect(hr, header_bg, 0);
        if ((int)m_headers.size() > c) ctx.draw_text(m_headers[c], {hx + 4, f.y + 4}, fg, 13);
    }

    for (int r = m_first_row; r <= m_last_row && r < m_rows; r++) {
        float y = f.y + m_header_height + (r - m_first_row) * m_row_height;
        float cx = f.x - m_scroll_x;
        for (int c = 0; c < m_cols; c++) {
            float cw = m_col_widths[c]; float col_x = cx; cx += cw;
            if (c < m_first_col || c > m_last_col) continue;
            Rect cr{col_x, y, cw, m_row_height};
            if (r == m_selected_row) ctx.draw_rect(cr, accent.darken(0.3f), 0);
            if (r % 2 == 0) ctx.draw_rect(cr, Color{0,0,0,0.05f}, 0);
            std::string cell = "(" + std::to_string(r) + "," + std::to_string(c) + ")";
            ctx.draw_text(cell, {col_x + 4, y + 3}, fg, 13);
        }
    }
}
bool TableView::on_event(InputEvent& ev) {
    Rect f = frame();
    if (ev.type == EventType::MouseWheel && f.contains(ev.pos)) {
        m_scroll_y = std::max(0.0f, m_scroll_y - ev.wheel_delta * 20); m_dirty = true; return true;
    }
    if (ev.type == EventType::MouseDown && f.contains(ev.pos)) {
        float y = ev.pos.y - (f.y + m_header_height);
        float x = ev.pos.x - f.x + m_scroll_x;
        int row = (int)(m_scroll_y + y) / (int)m_row_height;
        int col = 0; float xa = 0;
        for (int c = 0; c < m_cols; c++) { if (x < xa + m_col_widths[c]) { col = c; break; } xa += m_col_widths[c]; }
        if (row >= 0 && row < m_rows) {
            m_selected_row = row; m_selected_col = col;
            if (y < 0 && on_header_click) on_header_click(col);
            else if (on_cell_click) on_cell_click(row, col);
            return true;
        }
    }
    return Container::on_event(ev);
}

} // namespace ovui
