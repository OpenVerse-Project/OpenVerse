#include "Styling.h"
#include <algorithm>

namespace ovui {

void StyleSheet::add_rule(StyleRule rule) {
    rule.selector.compute_specificity();
    m_rules.push_back(std::move(rule));
}

std::vector<StyleValue> StyleSheet::resolve(const std::string& widget_type,
                                              const std::string& class_name,
                                              WidgetState state) const {
    std::vector<std::pair<uint32_t, const StyleValue*>> candidates;

    for (auto& rule : m_rules) {
        if (rule.selector.matches(widget_type, class_name, state)) {
            for (auto& val : rule.values) {
                candidates.push_back({rule.selector.specificity, &val});
            }
        }
    }

    std::stable_sort(candidates.begin(), candidates.end(),
                     [](auto& a, auto& b) { return a.first < b.first; });

    std::vector<StyleValue> result;
    uint32_t seen[(size_t)StylePropertyID::COUNT] = {};
    for (auto it = candidates.rbegin(); it != candidates.rend(); ++it) {
        uint32_t pid = (uint32_t)it->second->prop;
        if (!seen[pid]) {
            result.push_back(*it->second);
            seen[pid] = 1;
        }
    }
    return result;
}

void StyleSheet::clear() { m_rules.clear(); }

StyleEngine::StyleEngine() { set_default_theme(); }

void StyleEngine::load_sheet(std::shared_ptr<StyleSheet> sheet) {
    m_sheets.push_back(std::move(sheet));
}

void StyleEngine::replace_sheet(size_t i, std::shared_ptr<StyleSheet> sheet) {
    if (i < m_sheets.size()) m_sheets[i] = std::move(sheet);
}

void StyleEngine::clear_theme() { m_sheets.clear(); }

void StyleEngine::add_rule(const std::string& widget_type, WidgetState state,
                            std::initializer_list<StyleValue> vals) {
    if (m_sheets.empty()) m_sheets.push_back(std::make_shared<StyleSheet>());
    StyleRule rule;
    rule.selector.widget_type = widget_type;
    rule.selector.state_mask = state;
    rule.values = vals;
    m_sheets[0]->add_rule(std::move(rule));
}

void StyleEngine::add_rule_class(const std::string& widget_type, const std::string& klass,
                                  WidgetState state, std::initializer_list<StyleValue> vals) {
    if (m_sheets.empty()) m_sheets.push_back(std::make_shared<StyleSheet>());
    StyleRule rule;
    rule.selector.widget_type = widget_type;
    rule.selector.class_name = klass;
    rule.selector.state_mask = state;
    rule.values = vals;
    m_sheets[0]->add_rule(std::move(rule));
}

void StyleEngine::rebuild_default_theme(const std::vector<Color>& palette) {
    m_sheets.clear();
    auto sheet = std::make_shared<StyleSheet>();
    Color primary, surface, text_c, surface_alt, border_c, error_c;
    if (palette.size() >= 1) primary = palette[0]; else primary = Color::from_hex(0x4FC3F7FF);
    if (palette.size() >= 2) surface = palette[1]; else surface = Color::from_hex(0x1E1E2EFF);
    if (palette.size() >= 3) text_c = palette[2]; else text_c = Color::from_hex(0xCDD6F4FF);
    if (palette.size() >= 4) surface_alt = palette[3]; else surface_alt = Color::from_hex(0x282848FF);
    if (palette.size() >= 5) border_c = palette[4]; else border_c = Color::from_hex(0x3C3C5AFF);
    if (palette.size() >= 6) error_c = palette[5]; else error_c = Color::from_hex(0xF38BA8FF);

    auto add = [&](const char* type, WidgetState state, std::initializer_list<StyleValue> vals) {
        StyleRule rule;
        rule.selector.widget_type = type ? type : "";
        rule.selector.state_mask = state;
        rule.values = vals;
        sheet->add_rule(std::move(rule));
    };

    add("Box", WidgetState::None, {
        {StylePropertyID::BackgroundColor, {surface.r, surface.g, surface.b, surface.a}},
        {StylePropertyID::BorderRadius, {6, 0, 0, 0}},
    });
    add("Button", WidgetState::None, {
        {StylePropertyID::BackgroundColor, {primary.r, primary.g, primary.b, primary.a}},
        {StylePropertyID::Color, {text_c.r, text_c.g, text_c.b, text_c.a}},
        {StylePropertyID::BorderRadius, {6, 0, 0, 0}},
        {StylePropertyID::FontSize, {14, 0, 0, 0}},
        {StylePropertyID::PaddingTop, {8, 0, 0, 0}},
        {StylePropertyID::PaddingBottom, {8, 0, 0, 0}},
        {StylePropertyID::PaddingLeft, {16, 0, 0, 0}},
        {StylePropertyID::PaddingRight, {16, 0, 0, 0}},
    });
    add("Button", WidgetState::Hover, {
        {StylePropertyID::BackgroundColor, {primary.lighten(0.1f).r, primary.lighten(0.1f).g,
                                             primary.lighten(0.1f).b, primary.lighten(0.1f).a}},
    });
    add("Button", WidgetState::Pressed, {
        {StylePropertyID::BackgroundColor, {primary.darken(0.1f).r, primary.darken(0.1f).g,
                                             primary.darken(0.1f).b, primary.darken(0.1f).a}},
    });
    add("Text", WidgetState::None, {
        {StylePropertyID::Color, {text_c.r, text_c.g, text_c.b, text_c.a}},
        {StylePropertyID::FontSize, {13, 0, 0, 0}},
    });
    add("TextInput", WidgetState::None, {
        {StylePropertyID::BackgroundColor, {surface_alt.r, surface_alt.g, surface_alt.b, surface_alt.a}},
        {StylePropertyID::Color, {text_c.r, text_c.g, text_c.b, text_c.a}},
        {StylePropertyID::BorderRadius, {4, 0, 0, 0}},
        {StylePropertyID::BorderColor, {border_c.r, border_c.g, border_c.b, border_c.a}},
        {StylePropertyID::BorderWidth, {1, 0, 0, 0}},
        {StylePropertyID::FontSize, {13, 0, 0, 0}},
        {StylePropertyID::PaddingTop, {6, 0, 0, 0}},
        {StylePropertyID::PaddingBottom, {6, 0, 0, 0}},
        {StylePropertyID::PaddingLeft, {10, 0, 0, 0}},
        {StylePropertyID::PaddingRight, {10, 0, 0, 0}},
    });
    add("TextInput", WidgetState::Focused, {
        {StylePropertyID::BorderColor, {primary.r, primary.g, primary.b, primary.a}},
    });
    add("Slider", WidgetState::None, {
        {StylePropertyID::BackgroundColor, {surface_alt.r, surface_alt.g, surface_alt.b, surface_alt.a}},
        {StylePropertyID::BorderRadius, {2, 0, 0, 0}},
    });
    add("Window", WidgetState::None, {
        {StylePropertyID::BackgroundColor, {surface.darken(0.15f).r, surface.darken(0.15f).g,
                                             surface.darken(0.15f).b, surface.darken(0.15f).a}},
        {StylePropertyID::Color, {text_c.r, text_c.g, text_c.b, text_c.a}},
    });
    add("Checkbox", WidgetState::None, {
        {StylePropertyID::BackgroundColor, {surface_alt.r, surface_alt.g, surface_alt.b, surface_alt.a}},
        {StylePropertyID::Color, {text_c.r, text_c.g, text_c.b, text_c.a}},
    });

    m_sheets.push_back(sheet);
}

void StyleEngine::set_default_theme() {
    auto sheet = std::make_shared<StyleSheet>();

    auto add = [&](const char* type, const char* klass, WidgetState state,
                   std::initializer_list<StyleValue> vals) {
        StyleRule rule;
        rule.selector.widget_type = type ? type : "";
        rule.selector.class_name = klass ? klass : "";
        rule.selector.state_mask = state;
        rule.values = vals;
        sheet->add_rule(std::move(rule));
    };

    add("Box", nullptr, WidgetState::None, {
        {StylePropertyID::BackgroundColor, {0.117f, 0.117f, 0.180f, 1.0f}},
        {StylePropertyID::BorderRadius, {6, 0, 0, 0}},
    });

    add("Button", nullptr, WidgetState::None, {
        {StylePropertyID::BackgroundColor, {0.309f, 0.760f, 0.968f, 1.0f}},
        {StylePropertyID::Color, {1, 1, 1, 1}},
        {StylePropertyID::BorderRadius, {6, 0, 0, 0}},
        {StylePropertyID::FontSize, {14, 0, 0, 0}},
        {StylePropertyID::PaddingTop, {8, 0, 0, 0}},
        {StylePropertyID::PaddingBottom, {8, 0, 0, 0}},
        {StylePropertyID::PaddingLeft, {16, 0, 0, 0}},
        {StylePropertyID::PaddingRight, {16, 0, 0, 0}},
    });

    add("Button", nullptr, WidgetState::Hover, {
        {StylePropertyID::BackgroundColor, {0.360f, 0.788f, 0.972f, 1.0f}},
    });

    add("Button", nullptr, WidgetState::Pressed, {
        {StylePropertyID::BackgroundColor, {0.235f, 0.635f, 0.870f, 1.0f}},
    });

    add("Text", nullptr, WidgetState::None, {
        {StylePropertyID::Color, {0.803f, 0.839f, 0.956f, 1.0f}},
        {StylePropertyID::FontSize, {13, 0, 0, 0}},
    });

    add("TextInput", nullptr, WidgetState::None, {
        {StylePropertyID::BackgroundColor, {0.156f, 0.156f, 0.235f, 1.0f}},
        {StylePropertyID::Color, {0.803f, 0.839f, 0.956f, 1.0f}},
        {StylePropertyID::BorderRadius, {4, 0, 0, 0}},
        {StylePropertyID::BorderColor, {0.235f, 0.235f, 0.352f, 1.0f}},
        {StylePropertyID::BorderWidth, {1, 0, 0, 0}},
        {StylePropertyID::FontSize, {13, 0, 0, 0}},
        {StylePropertyID::PaddingTop, {6, 0, 0, 0}},
        {StylePropertyID::PaddingBottom, {6, 0, 0, 0}},
        {StylePropertyID::PaddingLeft, {10, 0, 0, 0}},
        {StylePropertyID::PaddingRight, {10, 0, 0, 0}},
    });

    add("TextInput", nullptr, WidgetState::Focused, {
        {StylePropertyID::BorderColor, {0.309f, 0.760f, 0.968f, 1.0f}},
    });

    add("Slider", nullptr, WidgetState::None, {
        {StylePropertyID::BackgroundColor, {0.156f, 0.156f, 0.235f, 1.0f}},
        {StylePropertyID::BorderRadius, {2, 0, 0, 0}},
    });

    add("Window", nullptr, WidgetState::None, {
        {StylePropertyID::BackgroundColor, {0.094f, 0.094f, 0.141f, 1.0f}},
        {StylePropertyID::Color, {0.803f, 0.839f, 0.956f, 1.0f}},
    });

    m_sheets.push_back(sheet);
}

std::vector<StyleValue> StyleEngine::resolve(const std::string& widget_type,
                                              const std::string& class_name,
                                              WidgetState state) const {
    std::vector<StyleValue> result;
    for (auto& sheet : m_sheets) {
        auto vals = sheet->resolve(widget_type, class_name, state);
        for (auto& v : vals) {
            bool replaced = false;
            for (auto& r : result) {
                if (r.prop == v.prop) { r = v; replaced = true; break; }
            }
            if (!replaced) result.push_back(v);
        }
    }
    return result;
}

Color StyleEngine::resolve_color(StylePropertyID prop, const std::vector<StyleValue>& values,
                                  Color fallback) const {
    for (auto& v : values) {
        if (v.prop == prop) return {v.f[0], v.f[1], v.f[2], v.f[3]};
    }
    return fallback;
}

float StyleEngine::resolve_float(StylePropertyID prop, const std::vector<StyleValue>& values,
                                  float fallback) const {
    for (auto& v : values) {
        if (v.prop == prop) return v.f[0];
    }
    return fallback;
}

EdgeInsets StyleEngine::resolve_edge_insets(const std::vector<StyleValue>& values) const {
    EdgeInsets e;
    for (auto& v : values) {
        switch (v.prop) {
            case StylePropertyID::PaddingTop: e.top = v.f[0]; break;
            case StylePropertyID::PaddingRight: e.right = v.f[0]; break;
            case StylePropertyID::PaddingBottom: e.bottom = v.f[0]; break;
            case StylePropertyID::PaddingLeft: e.left = v.f[0]; break;
            default: break;
        }
    }
    return e;
}

} // namespace ovui
