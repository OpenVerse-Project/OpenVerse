#pragma once

#include <Core/Types.h>
#include <Reactive/Reactive.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace ovui {

enum class StylePropertyID : uint32_t {
    BackgroundColor, Color, FontSize, FontWeight,
    BorderRadius, BorderWidth, BorderColor,
    PaddingTop, PaddingRight, PaddingBottom, PaddingLeft,
    MarginTop, MarginRight, MarginBottom, MarginLeft,
    Opacity, Transform, ShadowColor, ShadowOffsetX, ShadowOffsetY,
    ShadowBlur, MinWidth, MaxWidth, MinHeight, MaxHeight,
    Width, Height, Gap, CrossGap,
    COUNT
};

struct StyleValue {
    StylePropertyID prop;
    float f[4] = {0,0,0,0};
};

struct StyleSelector {
    std::string widget_type;
    std::string class_name;
    WidgetState state_mask = WidgetState::None;
    uint32_t specificity = 0;

    bool matches(const std::string& type, const std::string& klass, WidgetState state) const {
        if (!widget_type.empty() && widget_type != type) return false;
        if (!class_name.empty() && class_name != klass) return false;
        if (state_mask != WidgetState::None && !(state & state_mask)) return false;
        return true;
    }

    void compute_specificity() {
        specificity = 0;
        if (!widget_type.empty()) specificity += 1000;
        if (!class_name.empty()) specificity += 100;
        if (state_mask != WidgetState::None) specificity += 10;
    }
};

struct StyleRule {
    StyleSelector selector;
    std::vector<StyleValue> values;
};

class StyleSheet {
public:
    void add_rule(StyleRule rule);
    std::vector<StyleValue> resolve(const std::string& widget_type,
                                     const std::string& class_name,
                                     WidgetState state) const;
    void clear();
    size_t rule_count() const { return m_rules.size(); }

private:
    std::vector<StyleRule> m_rules;
};

class StyleEngine {
public:
    StyleEngine();

    void load_sheet(std::shared_ptr<StyleSheet> sheet);
    void replace_sheet(size_t i, std::shared_ptr<StyleSheet> sheet);
    void set_default_theme();
    void clear_theme();
    void add_rule(const std::string& widget_type, WidgetState state,
                  std::initializer_list<StyleValue> vals);
    void add_rule_class(const std::string& widget_type, const std::string& klass,
                        WidgetState state, std::initializer_list<StyleValue> vals);
    size_t sheet_count() const { return m_sheets.size(); }
    void rebuild_default_theme(const std::vector<Color>& palette);

    std::vector<StyleValue> resolve(const std::string& widget_type,
                                     const std::string& class_name,
                                     WidgetState state) const;

    Color resolve_color(StylePropertyID prop, const std::vector<StyleValue>& values,
                        Color fallback) const;
    float resolve_float(StylePropertyID prop, const std::vector<StyleValue>& values,
                        float fallback) const;
    EdgeInsets resolve_edge_insets(const std::vector<StyleValue>& values) const;

private:
    std::vector<std::shared_ptr<StyleSheet>> m_sheets;
};

} // namespace ovui
