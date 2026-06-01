#pragma once

#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Designer/PropertyInspector.h>
#include <Styling/Styling.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ovui {

struct ThemePalette {
    Color primary     = Color::from_hex(0x4FC3F7FF);
    Color surface     = Color::from_hex(0x1E1E2EFF);
    Color text        = Color::from_hex(0xCDD6F4FF);
    Color surface_alt = Color::from_hex(0x282848FF);
    Color border      = Color::from_hex(0x3C3C5AFF);
    Color error       = Color::from_hex(0xF38BA8FF);

    std::vector<Color> to_vec() const {
        return {primary, surface, text, surface_alt, border, error};
    }
    void from_vec(const std::vector<Color>& v) {
        if (v.size() >= 1) primary = v[0];
        if (v.size() >= 2) surface = v[1];
        if (v.size() >= 3) text = v[2];
        if (v.size() >= 4) surface_alt = v[3];
        if (v.size() >= 5) border = v[4];
        if (v.size() >= 6) error = v[5];
    }

    static ThemePalette dark() { return {}; }
    static ThemePalette light();
    static ThemePalette purple();
    static ThemePalette forest();
};

class ThemeEditor : public Container {
public:
    ThemeEditor();
    ~ThemeEditor() override = default;

    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    void set_palette(const ThemePalette& p);
    const ThemePalette& palette() const { return m_palette; }
    void apply_theme();

    void set_preview_widget_type(const std::string& type);
    const std::string& preview_widget_type() const { return m_preview_type; }

    void rebuild_preview();

    std::function<void(const ThemePalette&)> on_palette_changed;

    static std::vector<std::string> preset_names();
    static ThemePalette preset(const std::string& name);

private:
    ThemePalette m_palette;
    std::string m_preview_type = "Button";
    WidgetPtr m_preview;

    StyleEngine m_style_engine;
    PropertyInspector m_props;

    struct Swatch { std::string name; Color* color; Rect frame; };
    std::vector<Swatch> m_swatches;
    int m_hovered_swatch = -1;
    float m_swatch_size = 24;
    float m_swatch_gap = 4;
    float m_section_pad = 8;

    bool m_dirty = true;

    void build_swatches();
    void build_preview_panel();
    void build_property_panel();
    void rebuild_ui();
    int hit_swatch(Point p) const;
};

} // namespace ovui
