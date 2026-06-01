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
