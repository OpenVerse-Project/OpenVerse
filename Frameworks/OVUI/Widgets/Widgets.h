#pragma once

#include <Core/Types.h>
#include <Layout/FlexEngine.h>
#include <Styling/Styling.h>
#include <Reactive/Reactive.h>

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ovui {

class Widget;
class Window;
class PaintContext;

using WidgetPtr = std::shared_ptr<Widget>;

enum class EventType { MouseDown, MouseUp, MouseMove, MouseWheel, KeyDown, KeyUp, TextInput, Focus, Blur };

struct InputEvent {
    EventType type;
    Point pos;
    int button = 0, key = 0, modifiers = 0;
    float wheel_delta = 0;
    std::string text;
    bool handled = false;
};

class Widget : public std::enable_shared_from_this<Widget> {
public:
    Widget();
    virtual ~Widget() = default;

    void set_id(WidgetID i) { m_id = i; }
    WidgetID id() const { return m_id; }
    void set_widget_type(const std::string& t) { m_widget_type = t; }
    const std::string& widget_type() const { return m_widget_type; }
    void set_class(const std::string& c) { m_class_name = c; }
    const std::string& widget_class() const { return m_class_name; }

    void add_child(WidgetPtr child);
    void remove_child(WidgetPtr child);
    void remove_all_children();
    const std::vector<WidgetPtr>& children() const { return m_children; }
    WidgetPtr parent() const { return m_parent.lock(); }

    LayoutNode& layout_node() { return m_layout_node; }
    const LayoutNode& layout_node() const { return m_layout_node; }
    Rect frame() const { return m_layout_node.result.frame; }
    void set_frame(const Rect& f) { m_layout_node.result.frame = f; }

    WidgetState state() const { return m_state; }
    void set_state(WidgetState s);
    bool has_state(WidgetState s) const { return !!(m_state & s); }
    void set_enabled(bool e);
    bool is_enabled() const { return !has_state(WidgetState::Disabled); }
    void set_visible(bool v) { m_visible = v; }
    bool is_visible() const { return m_visible; }

    void capture_mouse() { m_mouse_capture = true; }
    void release_mouse() { m_mouse_capture = false; }
    bool has_mouse_capture() const { return m_mouse_capture; }

    StyleEngine& style_engine() { return m_style_engine; }
    std::vector<StyleValue> resolved_style() const;
    void invalidate_style() { m_style_dirty = true; }
    void update_layout_node();

    virtual void on_layout() {}
    virtual void on_paint(class PaintContext& ctx) {}
    virtual bool on_event(InputEvent& ev) { return false; }
    virtual void on_state_changed(WidgetState, WidgetState) {}

    Observable<Rect>& frame_observable() { return m_frame_obs; }

protected:
    std::vector<StyleValue> resolve_styles() const;

    WidgetID m_id = 0;
    std::string m_widget_type = "Widget";
    std::string m_class_name;
    WidgetState m_state = WidgetState::None;
    bool m_visible = true;
    mutable bool m_style_dirty = true;
    bool m_mouse_capture = false;

    LayoutNode m_layout_node;
    std::vector<WidgetPtr> m_children;
    std::weak_ptr<Widget> m_parent;
    StyleEngine m_style_engine;
    Observable<Rect> m_frame_obs;
    mutable std::vector<StyleValue> m_cached_style;
};

class PaintContext {
public:
    struct DrawCmd {
        enum Type { RectCmd, TextCmd, BorderCmd, LineCmd } type;
        Rect frame;
        Color color;
        std::string text;
        float radius_or_size = 0, border_width = 1;
    };

    void draw_rect(const Rect& r, Color c, float radius=0);
    void draw_text(const std::string& t, Point p, Color c, float size=13);
    void draw_border(const Rect& r, Color c, float w=1, float radius=0);
    void draw_circle(Point center, float radius, Color c);
    void draw_line(Point p1, Point p2, Color c, float w=1);
    const std::vector<DrawCmd>& commands() const { return m_cmds; }
    void reset() { m_cmds.clear(); }
    void set_viewport(Size vp) { m_viewport = vp; }
    Size viewport() const { return m_viewport; }
private:
    std::vector<DrawCmd> m_cmds;
    Size m_viewport{1920, 1080};
};

class Container : public Widget {
public:
    Container();
    void set_direction(FlexDirection d) { layout_node().direction = d; }
    void set_justify(JustifyContent j) { layout_node().justify = j; }
    void set_align(AlignItems a) { layout_node().align = a; }
    void set_gap(float g) { layout_node().gap = g; }
    void set_padding(EdgeInsets p) { layout_node().padding = p; }
};

class Box : public Widget {
public:
    Box();
    void on_paint(PaintContext& ctx) override;
    void set_color(Color c) { m_color = c; }
private:
    Color m_color{0,0,0,1};
};

class Text : public Widget {
public:
    Text();
    void on_paint(PaintContext& ctx) override;
    void set_text(const std::string& t) { m_text = t; }
    const std::string& get_text() const { return m_text; }
private:
    std::string m_text;
};

class Button : public Widget {
public:
    Button();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_text(const std::string& t) { m_label = t; }
    Observable<int>& click_obs() { return m_clicks; }
    std::function<void()> on_click;
private:
    std::string m_label;
    Observable<int> m_clicks{0};
};

class Slider : public Widget {
public:
    Slider();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_range(float min, float max) { m_min = min; m_max = max; }
    void set_value(float v) { m_value.set(v); }
    float value() const { return m_value.get(); }
    Observable<float>& value_obs() { return m_value; }
private:
    float m_min = 0, m_max = 100;
    Observable<float> m_value{50};
};

class TextInput : public Widget {
public:
    TextInput();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_text(const std::string& t);
    const std::string& text() const { return m_text.get(); }
    Observable<std::string>& text_obs() { return m_text; }
    void set_placeholder(const std::string& p) { m_placeholder = p; }
private:
    Observable<std::string> m_text;
    std::string m_placeholder;
    size_t m_cursor = 0;
    bool m_focused = false;
};

class ScrollArea : public Container {
public:
    ScrollArea();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_scroll_y(float y);
    float scroll_y() const { return m_scroll_y; }
private:
    float m_scroll_y = 0;
};

} // namespace ovui
