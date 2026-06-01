#pragma once

#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <memory>

namespace ovui {

enum class HandlePart : uint8_t {
    None = 0,
    TopLeft, Top, TopRight,
    Right,
    BottomRight, Bottom, BottomLeft,
    Left,
    COUNT
};

enum class DragMode : uint8_t {
    None,
    Move,
    Resize,
    BoxSelect
};

enum class AlignEdge : uint8_t {
    Left,
    CenterH,
    Right,
    Top,
    CenterV,
    Bottom
};

class DesignSurface : public Container {
public:
    using WidgetFactory = std::function<WidgetPtr()>;

    DesignSurface();
    ~DesignSurface() override = default;

    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    void select(WidgetPtr widget);
    void add_to_selection(WidgetPtr widget);
    void remove_from_selection(WidgetPtr widget);
    void deselect_all();
    WidgetPtr selected() const;
    const std::vector<WidgetPtr>& selection() const { return m_selection; }
    bool is_selected(WidgetPtr widget) const;

    void set_grid_size(float s) { m_grid_size = std::max(1.0f, s); }
    float grid_size() const { return m_grid_size; }
    void set_grid_enabled(bool e) { m_grid_enabled = e; }
    bool grid_enabled() const { return m_grid_enabled; }
    void set_snap_enabled(bool e) { m_snap_enabled = e; }
    bool snap_enabled() const { return m_snap_enabled; }

    void set_zoom(float z) { m_zoom = std::clamp(z, 0.05f, 10.0f); }
    float zoom() const { return m_zoom; }
    void set_pan(Point p) { m_pan_offset = p; }
    Point pan() const { return m_pan_offset; }

    Point canvas_to_surface(Point p) const;
    Point surface_to_canvas(Point p) const;

    void register_widget_type(const std::string& type, WidgetFactory factory);
    bool has_widget_type(const std::string& type) const;
    WidgetPtr create_widget(const std::string& type) const;

    void align_selected(AlignEdge edge);
    void distribute_horizontal();
    void distribute_vertical();

    void bring_to_front();
    void send_to_back();
    void nudge(float dx, float dy);

    void set_show_grid(bool s) { m_show_grid = s; }
    void set_show_handles(bool s) { m_show_handles = s; }
    void set_show_guides(bool s) { m_show_guides = s; }
    void set_min_widget_size(Size s) { m_min_size = s; }

    std::function<void(WidgetPtr)> on_selection_changed;
    std::function<void(WidgetPtr, Rect)> on_widget_moved;
    std::function<void(WidgetPtr, const std::string&)> on_widget_created;

private:
    struct AlignmentGuide {
        float position = 0;
        bool is_vertical = false;
    };

    std::vector<WidgetPtr> m_selection;
    DragMode m_drag_mode = DragMode::None;
    HandlePart m_active_handle = HandlePart::None;
    Point m_drag_start;
    Rect m_drag_original_frame;
    Point m_active_widget_original_pos;
    Rect m_overlay_frame;

    float m_grid_size = 8;
    bool m_grid_enabled = true;
    bool m_snap_enabled = true;
    bool m_show_grid = true;
    bool m_show_handles = true;
    bool m_show_guides = true;

    float m_zoom = 1.0f;
    Point m_pan_offset{0, 0};
    Size m_min_size{16, 16};

    std::unordered_map<std::string, WidgetFactory> m_factories;
    mutable std::vector<AlignmentGuide> m_cached_guides;

    HandlePart hit_test_handle(const Rect& frame, Point pos, float handle_size) const;
    Point snap_to_grid(Point p, bool force = false) const;
    void compute_alignment_guides(const Rect& target, WidgetPtr exclude);
    void draw_grid(PaintContext& ctx) const;
    void draw_selection_handles(PaintContext& ctx, const Rect& frame) const;
    void draw_alignment_guides(PaintContext& ctx) const;
    void draw_overlay(PaintContext& ctx) const;
    void draw_surface_background(PaintContext& ctx) const;
    WidgetPtr hit_test_widget(Point p) const;

    static constexpr float k_handle_size = 8;
    static constexpr float k_guide_threshold = 4;
};

} // namespace ovui
