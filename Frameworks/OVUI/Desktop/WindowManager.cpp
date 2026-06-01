#include "WindowManager.h"
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace ovui {

EnhancedWindow::EnhancedWindow() {
    set_widget_type("EnhancedWindow");
    set_padding({30, 0, 0, 0});
    m_minimizable = true;
    m_maximizable = true;
}

void EnhancedWindow::draw_title_buttons(PaintContext& ctx, Rect tb) const {
    float btn_size = 14;
    float y_off = (tb.height - btn_size) * 0.5f;
    float x = tb.right() - btn_size - 6;
    Color inactive{0.4f, 0.4f, 0.45f, 1};
    Color hover{0.6f, 0.6f, 0.65f, 1};
    Color close_col{0.95f, 0.34f, 0.42f, 1};

    if (is_closable()) {
        ctx.draw_text("\xe2\x9c\x95", {x, tb.y + y_off}, close_col, 14);
        x -= btn_size + 4;
    }
    if (m_maximizable) {
        ctx.draw_text(is_maximized() ? "\xe2\x96\xa1" : "\xe2\x96\xa0",
                      {x, tb.y + y_off}, is_maximized() ? inactive : hover, 12);
        x -= btn_size + 4;
    }
    if (m_minimizable) {
        ctx.draw_text("\xe2\x80\x94", {x, tb.y + y_off}, inactive, 14);
    }
}

void EnhancedWindow::on_paint(PaintContext& ctx) {
    if (is_minimized()) return;
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s,
                                             is_active() ? Color::from_hex(0x1A1A2EFF)
                                                         : Color::from_hex(0x161622FF));
    Color header = is_active() ? Color::from_hex(0x141420FF) : Color::from_hex(0x0E0E18FF);
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0xCDD6F4FF));
    Rect f = frame();

    ctx.draw_rect(f, bg, 8);
    ctx.draw_rect({f.x, f.y, f.width, titlebar_height()}, header, 8);

    ctx.draw_text(title(), {f.x + 10, f.y + 7}, fg, 14);
    draw_title_buttons(ctx, {f.x, f.y, f.width, titlebar_height()});

    if (is_active()) {
        ctx.draw_border(f, Color::from_hex(0x4FC3F7FF), 1, 8);
    } else {
        ctx.draw_border(f, Color{0.15f, 0.15f, 0.2f, 1}, 1, 8);
    }

    for (auto& child : children()) {
        if (child->is_visible()) child->on_paint(ctx);
    }
}

bool EnhancedWindow::on_event(InputEvent& ev) {
    if (is_minimized()) return false;
    Rect f = frame();
    Rect tb{f.x, f.y, f.width, titlebar_height()};

    if (has_mouse_capture()) {
        if (ev.type == EventType::MouseMove) {
            float nx = ev.pos.x - m_drag_start.x;
            float ny = ev.pos.y - m_drag_start.y;
            if (is_maximized()) { restore(); }
            set_frame({nx, ny, f.width, f.height});
            return true;
        }
        if (ev.type == EventType::MouseUp) {
            release_mouse();
            m_dragging = false;
            return true;
        }
        return true;
    }

    if (ev.type == EventType::MouseDown && tb.contains(ev.pos)) {
        float btn_size = 14;
        float x_check = f.right() - btn_size - 6;

        if (is_closable() && ev.pos.x > x_check) {
            if (on_close) on_close();
            return true;
        }
        if (m_maximizable) {
            x_check -= btn_size + 4;
            if (ev.pos.x > x_check && ev.pos.x < x_check + btn_size) {
                if (is_maximized()) { restore(); if (on_restore) on_restore(); }
                else { maximize({800, 600}); if (on_maximize) on_maximize(); }
                return true;
            }
        }
        if (m_minimizable) {
            x_check -= btn_size + 4;
            if (ev.pos.x > x_check && ev.pos.x < x_check + btn_size) {
                minimize();
                if (on_minimize) on_minimize();
                return true;
            }
        }

        m_dragging = true;
        m_drag_start = {ev.pos.x - f.x, ev.pos.y - f.y};
        capture_mouse();
        return true;
    }

    if (ev.type == EventType::MouseDown && !tb.contains(ev.pos) && f.contains(ev.pos)) {
        return true;
    }

    for (auto& child : children()) {
        if (child->is_visible() && child->on_event(ev)) return true;
    }
    return false;
}

void EnhancedWindow::minimize() {
    m_state |= WindowState::Minimized;
    m_state &= ~WindowState::Maximized;
}

void EnhancedWindow::maximize(Size ws) {
    m_restore_frame = frame();
    set_frame({0, 0, ws.width, ws.height});
    m_state |= WindowState::Maximized;
    m_state &= ~WindowState::Minimized;
}

void EnhancedWindow::restore() {
    set_frame(m_restore_frame);
    m_state &= ~(WindowState::Minimized | WindowState::Maximized);
}

void EnhancedWindow::set_active(bool a) {
    if (a) m_state |= WindowState::Active;
    else m_state &= ~WindowState::Active;
}

ToolWindow::ToolWindow() {
    set_widget_type("ToolWindow");
    set_titlebar_height(24);
    set_padding({24, 0, 0, 0});
}

void ToolWindow::on_paint(PaintContext& ctx) {
    if (is_minimized()) return;
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s,
                                             Color::from_hex(0x161622FF));
    Color header = Color::from_hex(0x0E0E18FF);
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0x9CA3B0FF));
    Rect f = frame();

    ctx.draw_rect(f, bg, 4);
    ctx.draw_rect({f.x, f.y, f.width, titlebar_height()}, header, 4);
    ctx.draw_text(title(), {f.x + 8, f.y + 3}, fg, 12);

    if (is_closable())
        ctx.draw_text("\xe2\x9c\x95", {f.right() - 18, f.y + 3},
                      Color::from_hex(0xF38BA8FF), 12);

    for (auto& child : children()) {
        if (child->is_visible()) child->on_paint(ctx);
    }
}

FloatingWindow::FloatingWindow() {
    set_widget_type("FloatingWindow");
    m_state |= WindowState::Floating;
    set_resizable(true);
}

void FloatingWindow::set_always_on_top(bool v) {
    m_always_on_top = v;
}

void FloatingWindow::on_paint(PaintContext& ctx) {
    Rect f = frame();
    ctx.draw_rect({f.x - 2, f.y - 2, f.width + 4, f.height + 4},
                  Color{0, 0, 0, 0.3f}, 10);
    EnhancedWindow::on_paint(ctx);
}

bool FloatingWindow::on_event(InputEvent& ev) {
    if (is_minimized()) return false;
    Rect f = frame();
    Rect resize_border = {f.x - 4, f.y - 4, f.width + 8, f.height + 8};

    if (ev.type == EventType::MouseDown && resize_border.contains(ev.pos) && !frame().contains(ev.pos)) {
        return true;
    }
    return EnhancedWindow::on_event(ev);
}

DockWindow::DockWindow() {
    set_widget_type("DockWindow");
    m_state |= WindowState::Docked;
    set_titlebar_height(24);
}

void DockWindow::on_paint(PaintContext& ctx) {
    if (is_minimized()) return;
    auto s = resolve_styles();
    Color bg = m_style_engine.resolve_color(StylePropertyID::BackgroundColor, s,
                                             Color::from_hex(0x13131EFF));
    Color header = Color::from_hex(0x0A0A14FF);
    Color fg = m_style_engine.resolve_color(StylePropertyID::Color, s, Color::from_hex(0x9CA3B0FF));
    Color dock_tab = Color::from_hex(0x1A1A2EFF);
    Rect f = frame();

    switch (m_dock_side) {
        case DockSide::Left:
            ctx.draw_rect({f.x, f.y, dock_tab_width, f.height}, dock_tab, 0);
            ctx.draw_text(title(), {f.x + dock_tab_width + 6, f.y + 4}, fg, 11);
            break;
        case DockSide::Right:
            ctx.draw_rect({f.right() - dock_tab_width, f.y, dock_tab_width, f.height}, dock_tab, 0);
            ctx.draw_text(title(), {f.x + 6, f.y + 4}, fg, 11);
            break;
        case DockSide::Bottom:
            ctx.draw_rect({f.x, f.bottom() - dock_tab_width, f.width, dock_tab_width}, dock_tab, 0);
            ctx.draw_text(title(), {f.x + 6, f.y + 4}, fg, 11);
            break;
        default:
            ctx.draw_rect({f.x, f.y, f.width, titlebar_height()}, header, 0);
            ctx.draw_text(title(), {f.x + 8, f.y + 4}, fg, 12);
            break;
    }

    for (auto& child : children()) {
        if (child->is_visible()) child->on_paint(ctx);
    }
}

bool DockWindow::on_event(InputEvent& ev) {
    if (is_minimized()) return false;
    return EnhancedWindow::on_event(ev);
}

ModalDialog::ModalDialog() {
    set_widget_type("ModalDialog");
    set_modal(true);
    set_blocking(true);
}

void ModalDialog::on_paint(PaintContext& ctx) {
    Size vp = ctx.viewport();
    ctx.draw_rect({0, 0, vp.width, vp.height}, Color{0, 0, 0, 0.6f}, 0);
    Window::on_paint(ctx);
}

bool ModalDialog::on_event(InputEvent& ev) {
    if (!is_showing()) return false;
    Rect f = frame();
    if (m_blocking && !f.contains(ev.pos)) {
        return true;
    }
    return Window::on_event(ev);
}

void ModalDialog::show_modal(std::shared_ptr<ModalDialog> dialog,
                              std::shared_ptr<WindowManager> wm) {
    if (wm) wm->show_modal(dialog);
}

WindowManager::WindowManager() {}
WindowManager::~WindowManager() { m_windows.clear(); }

void WindowManager::set_root_container(WidgetPtr root) { m_root = root; }

void WindowManager::add_window(std::shared_ptr<EnhancedWindow> win) {
    if (!win) return;
    auto it = std::find(m_windows.begin(), m_windows.end(), win);
    if (it != m_windows.end()) return;
    win->set_window_id(next_window_id());
    m_windows.push_back(win);
    rebuild_z_order();
    if (on_window_added) on_window_added(win);
}

void WindowManager::remove_window(std::shared_ptr<EnhancedWindow> win) {
    auto it = std::find(m_windows.begin(), m_windows.end(), win);
    if (it == m_windows.end()) return;
    if (m_active.lock() == win) m_active.reset();
    m_windows.erase(it);
    rebuild_z_order();
    if (on_window_removed) on_window_removed(win);
}

bool WindowManager::has_window(std::shared_ptr<EnhancedWindow> win) const {
    return std::find(m_windows.begin(), m_windows.end(), win) != m_windows.end();
}

void WindowManager::bring_to_front(std::shared_ptr<EnhancedWindow> win) {
    auto it = std::find(m_windows.begin(), m_windows.end(), win);
    if (it == m_windows.end()) return;
    m_windows.erase(it);
    m_windows.push_back(win);
    rebuild_z_order();
}

void WindowManager::send_to_back(std::shared_ptr<EnhancedWindow> win) {
    auto it = std::find(m_windows.begin(), m_windows.end(), win);
    if (it == m_windows.end()) return;
    m_windows.erase(it);
    m_windows.insert(m_windows.begin(), win);
    rebuild_z_order();
}

void WindowManager::show_modal(std::shared_ptr<ModalDialog> dialog) {
    m_modal_stack.push_back(dialog);
    dialog->show(m_root, {1920, 1080});
}

void WindowManager::dismiss_modal() {
    if (m_modal_stack.empty()) return;
    m_modal_stack.back()->dismiss();
    m_modal_stack.pop_back();
}

void WindowManager::set_active_window(std::shared_ptr<EnhancedWindow> win) {
    if (!win) return;
    for (auto& w : m_windows) w->set_active(w == win);
    m_active = win;
    bring_to_front(win);
    if (on_window_focused) on_window_focused(win);
}

std::shared_ptr<EnhancedWindow> WindowManager::active_window() const {
    return m_active.lock();
}

std::shared_ptr<EnhancedWindow> WindowManager::window_at(Point screen_pos) const {
    for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it) {
        auto& w = *it;
        if (!w->is_minimized() && w->frame().contains(screen_pos))
            return w;
    }
    return nullptr;
}

std::vector<std::shared_ptr<EnhancedWindow>> WindowManager::windows() const {
    return m_windows;
}

std::vector<std::shared_ptr<EnhancedWindow>> WindowManager::floating_windows() const {
    std::vector<std::shared_ptr<EnhancedWindow>> result;
    for (auto& w : m_windows)
        if (w->window_state() & WindowState::Floating)
            result.push_back(w);
    return result;
}

std::vector<std::shared_ptr<EnhancedWindow>> WindowManager::docked_windows() const {
    std::vector<std::shared_ptr<EnhancedWindow>> result;
    for (auto& w : m_windows)
        if (w->window_state() & WindowState::Docked)
            result.push_back(w);
    return result;
}

bool WindowManager::dispatch_event(InputEvent& ev) {
    if (has_active_modal()) {
        return m_modal_stack.back()->on_event(ev);
    }

    for (auto it = m_windows.rbegin(); it != m_windows.rend(); ++it) {
        auto& w = *it;
        if (w->is_minimized()) continue;

        bool handled = false;
        Rect f = w->frame();
        Rect resize_area = {f.x - 4, f.y - 4, f.width + 8, f.height + 8};

        if (ev.type == EventType::MouseDown || ev.type == EventType::MouseUp ||
            ev.type == EventType::MouseMove) {
            if (f.contains(ev.pos) || resize_area.contains(ev.pos)) {
                if (ev.type == EventType::MouseDown && !has_active_modal()) {
                    set_active_window(w);
                }
                handled = w->on_event(ev);
            }
        }

        if (handled) return true;
    }
    return false;
}

void WindowManager::update_layout(Size workspace) {
    for (auto& w : m_windows) {
        if (w->is_maximized()) {
            w->maximize(workspace);
        }
    }
}

void WindowManager::rebuild_z_order() {}

bool WindowManager::handle_title_bar_event(std::shared_ptr<EnhancedWindow> win, InputEvent& ev) {
    if (!win) return false;
    return false;
}

std::string serialize_window_layout(const std::vector<std::shared_ptr<EnhancedWindow>>& windows) {
    std::ostringstream os;
    os << windows.size() << "\n";
    for (auto& w : windows) {
        Rect f = w->frame();
        os << w->title() << "|" << f.x << "|" << f.y << "|" << f.width << "|" << f.height
           << "|" << w->window_state() << "\n";
    }
    return os.str();
}

void deserialize_window_layout(const std::string& data,
                                std::vector<WindowRect>& out_rects,
                                std::vector<uint32_t>& out_states) {
    std::istringstream is(data);
    size_t count;
    is >> count;
    is.ignore();
    for (size_t i = 0; i < count; i++) {
        std::string line;
        std::getline(is, line);
        std::replace(line.begin(), line.end(), '|', ' ');
        std::istringstream ls(line);
        std::string title;
        WindowRect wr;
        uint32_t st;
        ls >> title >> wr.x >> wr.y >> wr.width >> wr.height >> st;
        out_rects.push_back(wr);
        out_states.push_back(st);
    }
}

Workspace::Workspace() {
    set_widget_type("Workspace");
}

void Workspace::set_window_manager(std::shared_ptr<WindowManager> wm) {
    m_wm = wm ? wm : std::make_shared<WindowManager>();
    m_wm->set_root_container(shared_from_this());
}

void Workspace::add_floating_window(std::shared_ptr<EnhancedWindow> win) {
    if (!win) return;
    if (!m_wm) m_wm = std::make_shared<WindowManager>();
    m_floating.push_back(win);
    add_child(win);
    m_wm->add_window(win);
}

void Workspace::add_docked_window(std::shared_ptr<DockWindow> win,
                                   DockWindow::DockSide side, float ratio) {
    if (!win) return;
    if (!m_wm) m_wm = std::make_shared<WindowManager>();
    win->set_dock_side(side);
    win->set_split_ratio(ratio);
    m_docked.push_back(win);
    m_dock_slots.push_back({side, ratio, win->window_id()});
    add_child(win);
    m_wm->add_window(win);
}

Rect Workspace::client_area() const {
    Rect f = frame();
    float left = 0, right = 0, top = m_menubar_h, bottom = 0;
    for (auto& dw : m_docked) {
        switch (dw->dock_side()) {
            case DockWindow::DockSide::Left:
                left = std::max(left, dw->frame().width);
                break;
            case DockWindow::DockSide::Right:
                right = std::max(right, dw->frame().width);
                break;
            case DockWindow::DockSide::Bottom:
                bottom = std::max(bottom, dw->frame().height);
                break;
            default: break;
        }
    }
    return {f.x + left, f.y + top, f.width - left - right, f.height - top - bottom};
}

void Workspace::layout_docked() {
    Rect ca = client_area();
    float left_x = frame().x;
    float right_x = frame().right();
    float bottom_y = frame().bottom();

    for (auto& dw : m_docked) {
        switch (dw->dock_side()) {
            case DockWindow::DockSide::Left: {
                float w = frame().width * dw->split_ratio();
                dw->set_frame({left_x, ca.y, w, ca.height});
                left_x += w;
                break;
            }
            case DockWindow::DockSide::Right: {
                float w = frame().width * dw->split_ratio();
                dw->set_frame({right_x - w, ca.y, w, ca.height});
                right_x -= w;
                break;
            }
            case DockWindow::DockSide::Bottom: {
                float h = frame().height * dw->split_ratio();
                dw->set_frame({ca.x, bottom_y - h, ca.width, h});
                bottom_y -= h;
                break;
            }
            case DockWindow::DockSide::Fill:
                dw->set_frame(ca);
                break;
            default: break;
        }
    }
}

void Workspace::layout_floating() {
    for (auto& fw : m_floating) {
        if (fw->is_maximized()) {
            fw->maximize({frame().width, frame().height});
        }
    }
}

void Workspace::on_paint(PaintContext& ctx) {
    Rect f = frame();
    ctx.draw_rect(f, Color{0.08f, 0.08f, 0.12f, 1}, 0);

    layout_docked();
    layout_floating();

    for (auto& child : children()) {
        if (child->is_visible()) child->on_paint(ctx);
    }
}

bool Workspace::on_event(InputEvent& ev) {
    if (!m_wm) return false;
    if (m_wm->dispatch_event(ev)) return true;

    for (auto it = children().rbegin(); it != children().rend(); ++it) {
        if ((*it)->is_visible() && (*it)->on_event(ev)) return true;
    }
    return false;
}

void Workspace::save_layout(const std::string& path) {
    if (!m_wm) return;
    auto data = serialize_window_layout(m_wm->windows());
    std::ofstream f(path);
    if (f) f << data;
}

bool Workspace::load_layout(const std::string& path) {
    std::ifstream f(path);
    if (!f) return false;
    std::string data((std::istreambuf_iterator<char>(f)),
                      std::istreambuf_iterator<char>());
    std::vector<WindowRect> rects;
    std::vector<uint32_t> states;
    deserialize_window_layout(data, rects, states);
    return !rects.empty();
}

} // namespace ovui
