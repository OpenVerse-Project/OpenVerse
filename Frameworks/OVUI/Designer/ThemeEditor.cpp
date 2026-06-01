#include "ThemeEditor.h"
#include <algorithm>

namespace ovui {

ThemePalette ThemePalette::light() {
    ThemePalette p;
    p.primary     = Color::from_hex(0x1E88E5FF);
    p.surface     = Color::from_hex(0xF5F5F5FF);
    p.text        = Color::from_hex(0x1A1A2EFF);
    p.surface_alt = Color::from_hex(0xE0E0E0FF);
    p.border      = Color::from_hex(0xBDBDBDFF);
    p.error       = Color::from_hex(0xE53935FF);
    return p;
}

ThemePalette ThemePalette::purple() {
    ThemePalette p;
    p.primary     = Color::from_hex(0xAB47BCFF);
    p.surface     = Color::from_hex(0x1A1A2EFF);
    p.text        = Color::from_hex(0xE1BEE7FF);
    p.surface_alt = Color::from_hex(0x252540FF);
    p.border      = Color::from_hex(0x4A148CFF);
    p.error       = Color::from_hex(0xEF9A9AFF);
    return p;
}

ThemePalette ThemePalette::forest() {
    ThemePalette p;
    p.primary     = Color::from_hex(0x66BB6AFF);
    p.surface     = Color::from_hex(0x1B2E1BFF);
    p.text        = Color::from_hex(0xC8E6C9FF);
    p.surface_alt = Color::from_hex(0x243824FF);
    p.border      = Color::from_hex(0x2E7D32FF);
    p.error       = Color::from_hex(0xEF5350FF);
    return p;
}

std::vector<std::string> ThemeEditor::preset_names() {
    return {"Dark", "Light", "Purple", "Forest"};
}

ThemePalette ThemeEditor::preset(const std::string& name) {
    if (name == "Light") return ThemePalette::light();
    if (name == "Purple") return ThemePalette::purple();
    if (name == "Forest") return ThemePalette::forest();
    return ThemePalette::dark();
}

ThemeEditor::ThemeEditor() {
    set_widget_type("ThemeEditor");
    layout_node().direction = FlexDirection::Column;
    layout_node().padding = {4, 4, 4, 4};
    layout_node().gap = 4;
    m_dirty = true;
}

void ThemeEditor::set_palette(const ThemePalette& p) {
    m_palette = p;
    m_style_engine.rebuild_default_theme(m_palette.to_vec());
    m_dirty = true;
}

void ThemeEditor::apply_theme() {
    m_style_engine.rebuild_default_theme(m_palette.to_vec());
    build_preview_panel();
}

void ThemeEditor::set_preview_widget_type(const std::string& type) {
    m_preview_type = type;
    m_dirty = true;
}

void ThemeEditor::rebuild_preview() {
    build_preview_panel();
}

void ThemeEditor::build_swatches() {
    m_swatches.clear();
    struct NamedColor { std::string name; Color* ptr; };
    NamedColor colors[] = {
        {"Primary", &m_palette.primary},
        {"Surface", &m_palette.surface},
        {"Text", &m_palette.text},
        {"Surface Alt", &m_palette.surface_alt},
        {"Border", &m_palette.border},
        {"Error", &m_palette.error},
    };

    float x = m_section_pad;
    float y = m_section_pad;
    for (auto& nc : colors) {
        m_swatches.push_back({nc.name, nc.ptr, {x, y, m_swatch_size, m_swatch_size}});
        x += m_swatch_size + m_swatch_gap;
    }
}

void ThemeEditor::build_preview_panel() {
    m_preview = nullptr;
    m_style_engine.rebuild_default_theme(m_palette.to_vec());

    if (m_preview_type == "Button") {
        auto btn = std::make_shared<Button>();
        btn->set_text("Preview Button");
        btn->set_frame({0, 0, 160, 40});
        m_preview = btn;
    } else if (m_preview_type == "TextInput") {
        auto ti = std::make_shared<TextInput>();
        ti->set_text("Preview text");
        ti->set_frame({0, 0, 200, 32});
        m_preview = ti;
    } else if (m_preview_type == "Checkbox") {
        auto cb = std::make_shared<Checkbox>();
        cb->set_label("Preview checkbox");
        cb->set_checked(true);
        cb->set_frame({0, 0, 200, 24});
        m_preview = cb;
    } else if (m_preview_type == "Slider") {
        auto sl = std::make_shared<Slider>();
        sl->set_value(70);
        sl->set_frame({0, 0, 200, 24});
        m_preview = sl;
    } else if (m_preview_type == "Window") {
        auto win = std::make_shared<Window>();
        win->set_title("Preview Window");
        win->set_frame({0, 0, 300, 120});
        auto inner_btn = std::make_shared<Button>();
        inner_btn->set_text("OK");
        inner_btn->set_frame({100, 45, 100, 30});
        win->add_child(inner_btn);
        m_preview = win;
    } else {
        auto box = std::make_shared<Box>();
        box->set_frame({0, 0, 100, 60});
        m_preview = box;
    }
}

void ThemeEditor::build_property_panel() {
    return;
}

void ThemeEditor::rebuild_ui() {
    remove_all_children();
    m_swatches.clear();
    build_swatches();
    build_preview_panel();
    m_dirty = false;
}

int ThemeEditor::hit_swatch(Point p) const {
    for (size_t i = 0; i < m_swatches.size(); i++) {
        if (m_swatches[i].frame.contains(p)) return (int)i;
    }
    return -1;
}

void ThemeEditor::on_paint(PaintContext& ctx) {
    if (m_dirty) rebuild_ui();
    Rect f = frame();

    ctx.draw_rect(f, Color{0.1f, 0.1f, 0.14f, 1}, 4);

    float header_h = m_swatch_size + m_section_pad * 2;
    ctx.draw_text("Theme Palette", {f.x + 8, f.y + 4}, Color{0.5f, 0.5f, 0.6f, 1}, 11);

    for (auto& sw : m_swatches) {
        Rect r = {f.x + sw.frame.x, f.y + header_h * 0.3f, m_swatch_size, m_swatch_size};
        ctx.draw_rect(r, *sw.color, 4);
        ctx.draw_border(r, Color{0.3f, 0.3f, 0.35f, 1}, 1, 4);
        if (m_hovered_swatch >= 0 && &m_swatches[m_hovered_swatch] == &sw) {
            ctx.draw_border(r, Color{1, 1, 1, 0.6f}, 2, 4);
        }
    }

    for (size_t i = 0; i < m_swatches.size(); i++) {
        ctx.draw_text(m_swatches[i].name, {f.x + m_section_pad + i * (m_swatch_size + m_swatch_gap),
                                            f.y + header_h + 2},
                      Color{0.6f, 0.6f, 0.65f, 1}, 10);
    }

    float preview_y = f.y + header_h + 20;
    ctx.draw_text("Live Preview — " + m_preview_type,
                  {f.x + 8, preview_y}, Color{0.5f, 0.5f, 0.6f, 1}, 11);

    if (m_preview) {
        Rect pf = m_preview->frame();
        m_preview->set_frame({f.x + 16, preview_y + 18, pf.width, pf.height});
        m_preview->on_paint(ctx);
    }
}

bool ThemeEditor::on_event(InputEvent& ev) {
    if (ev.type == EventType::MouseDown) {
        Rect f = frame();
        float header_h = m_swatch_size + m_section_pad * 2;
        Point local = {ev.pos.x - f.x, ev.pos.y - f.y};

        if (local.y >= m_section_pad && local.y < m_section_pad + m_swatch_size) {
            for (size_t i = 0; i < m_swatches.size(); i++) {
                Rect r = {m_swatches[i].frame.x, m_section_pad, m_swatch_size, m_swatch_size};
                if (r.contains(local)) {
                    auto& sw = m_swatches[i];
                    Color& c = *sw.color;
                    c.r = (c.r > 0.5f) ? 0.2f : 0.9f;
                    m_style_engine.rebuild_default_theme(m_palette.to_vec());
                    build_preview_panel();
                    if (on_palette_changed) on_palette_changed(m_palette);
                    m_dirty = true;
                    return true;
                }
            }
        }
    }
    return false;
}

} // namespace ovui
