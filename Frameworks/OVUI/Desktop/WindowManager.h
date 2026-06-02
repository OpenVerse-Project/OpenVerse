#pragma once

#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>
#include <fstream>
#include <sstream>

namespace ovui {

struct WindowState {
    enum Flag : uint32_t {
        None      = 0,
        Minimized = 1 << 0,
        Maximized = 1 << 1,
        Floating  = 1 << 2,
        Docked    = 1 << 3,
        Modal     = 1 << 4,
        Active    = 1 << 5,
        Closing   = 1 << 6,
    };
};

struct WindowRect {
    float x = 0, y = 0, width = 400, height = 300;
    static WindowRect from_ovui(const Rect& r) { return {r.x, r.y, r.width, r.height}; }
    Rect to_ovui() const { return {x, y, width, height}; }
};

class EnhancedWindow : public Window {
public:
    EnhancedWindow();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    void minimize();
    void maximize(Size workspace);
    void restore();
    bool is_minimized() const { return m_state & WindowState::Minimized; }
    bool is_maximized() const { return m_state & WindowState::Maximized; }

    void set_minimizable(bool v) { m_minimizable = v; }
    void set_maximizable(bool v) { m_maximizable = v; }
    void set_restore_frame(const Rect& r) { m_restore_frame = r; }
    Rect restore_frame() const { return m_restore_frame; }

    void set_window_id(int id) { m_window_id = id; }
    int window_id() const { return m_window_id; }

    void set_active(bool a);
    bool is_active() const { return m_state & WindowState::Active; }

    void set_window_state(uint32_t s) { m_state = s; }
    uint32_t window_state() const { return m_state; }

    std::function<void()> on_minimize;
    std::function<void()> on_maximize;
    std::function<void()> on_restore;

protected:
    uint32_t m_state = WindowState::None;
    Rect m_restore_frame{100, 100, 600, 400};
    bool m_minimizable = true;
    bool m_maximizable = true;
    int m_window_id = -1;
    void draw_title_buttons(PaintContext& ctx, Rect title_bar) const;
};

class ToolWindow : public EnhancedWindow {
public:
    ToolWindow();
    void on_paint(PaintContext& ctx) override;
    void set_compact(bool c) { m_compact = c; }
private:
    bool m_compact = true;
};

class FloatingWindow : public EnhancedWindow {
public:
    FloatingWindow();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_always_on_top(bool v);
    bool is_always_on_top() const { return m_always_on_top; }
    void set_dockable(bool v) { m_dockable = v; }
    bool is_dockable() const { return m_dockable; }
private:
    bool m_always_on_top = true;
    bool m_dockable = true;
};

class DockWindow : public EnhancedWindow {
public:
    DockWindow();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    enum class DockSide { Left, Right, Top, Bottom, Fill };
    void set_dock_side(DockSide s) { m_dock_side = s; }
    DockSide dock_side() const { return m_dock_side; }
    void set_split_ratio(float r) { m_split_ratio = std::clamp(r, 0.1f, 0.9f); }
    float split_ratio() const { return m_split_ratio; }

    static constexpr float dock_tab_width = 24;

private:
    DockSide m_dock_side = DockSide::Fill;
    float m_split_ratio = 0.25f;
    bool m_resizing = false;
    float m_drag_origin = 0;
};

class ModalDialog : public Dialog {
public:
    ModalDialog();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    void set_blocking(bool b) { m_blocking = b; }
    bool is_blocking() const { return m_blocking; }

    static void show_modal(std::shared_ptr<ModalDialog> dialog,
                           std::shared_ptr<class WindowManager> wm);
private:
    bool m_blocking = true;
};

class WindowManager : public std::enable_shared_from_this<WindowManager> {
public:
    WindowManager();
    ~WindowManager();

    void set_root_container(WidgetPtr root);

    void add_window(std::shared_ptr<EnhancedWindow> win);
    void remove_window(std::shared_ptr<EnhancedWindow> win);
    bool has_window(std::shared_ptr<EnhancedWindow> win) const;

    void bring_to_front(std::shared_ptr<EnhancedWindow> win);
    void send_to_back(std::shared_ptr<EnhancedWindow> win);

    void show_modal(std::shared_ptr<ModalDialog> dialog);
    void dismiss_modal();
    bool has_active_modal() const { return !m_modal_stack.empty(); }

    void set_active_window(std::shared_ptr<EnhancedWindow> win);
    std::shared_ptr<EnhancedWindow> active_window() const;
    std::shared_ptr<EnhancedWindow> window_at(Point screen_pos) const;

    std::vector<std::shared_ptr<EnhancedWindow>> windows() const;
    std::vector<std::shared_ptr<EnhancedWindow>> floating_windows() const;
    std::vector<std::shared_ptr<EnhancedWindow>> docked_windows() const;

    int window_count() const { return (int)m_windows.size(); }
    int next_window_id() { return m_next_id++; }

    bool dispatch_event(InputEvent& ev);
    void update_layout(Size workspace);

    std::function<void(std::shared_ptr<EnhancedWindow>)> on_window_added;
    std::function<void(std::shared_ptr<EnhancedWindow>)> on_window_removed;
    std::function<void(std::shared_ptr<EnhancedWindow>)> on_window_focused;

private:
    WidgetPtr m_root;
    std::vector<std::shared_ptr<EnhancedWindow>> m_windows;
    std::weak_ptr<EnhancedWindow> m_active;
    std::vector<std::shared_ptr<ModalDialog>> m_modal_stack;
    int m_next_id = 1;

    void rebuild_z_order();
    bool handle_title_bar_event(std::shared_ptr<EnhancedWindow> win, InputEvent& ev);
};

std::string serialize_window_layout(const std::vector<std::shared_ptr<EnhancedWindow>>& windows);
void deserialize_window_layout(const std::string& data,
                                std::vector<WindowRect>& out_rects,
                                std::vector<uint32_t>& out_states);

class Workspace : public Container {
public:
    Workspace();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    void set_window_manager(std::shared_ptr<WindowManager> wm);
    std::shared_ptr<WindowManager> window_manager() const {
        if (!m_wm) const_cast<Workspace*>(this)->m_wm = std::make_shared<WindowManager>();
        return m_wm;
    }

    void add_floating_window(std::shared_ptr<EnhancedWindow> win);
    void add_docked_window(std::shared_ptr<DockWindow> win,
                           DockWindow::DockSide side, float ratio = 0.25f);

    void save_layout(const std::string& path);
    bool load_layout(const std::string& path);

    void set_menubar_height(float h) { m_menubar_h = h; }

    void relayout();

    std::function<void()> on_layout_changed;

private:
    std::shared_ptr<WindowManager> m_wm;
    std::vector<std::shared_ptr<EnhancedWindow>> m_floating;
    std::vector<std::shared_ptr<DockWindow>> m_docked;
    float m_menubar_h = 0;

    Rect client_area() const;
    void layout_docked();
    void layout_floating();

    struct DockSlot { DockWindow::DockSide side; float ratio; int id; };
    std::vector<DockSlot> m_dock_slots;
};

} // namespace ovui
