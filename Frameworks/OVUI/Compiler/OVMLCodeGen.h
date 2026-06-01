#pragma once

#include <Compiler/OVMLParser.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Styling/Styling.h>
#include <string>
#include <functional>
#include <unordered_map>
#include <memory>

namespace ovui {

class OVMLCodeGen {
public:
    OVMLCodeGen();

    void register_widget_factory(
        const std::string& type,
        std::function<WidgetPtr(const OVMLNode&)> factory
    );

    WidgetPtr generate(const OVMLNode& root);

    std::shared_ptr<StyleEngine> style_engine() { return m_style_engine; }
    void set_style_engine(std::shared_ptr<StyleEngine> se) { m_style_engine = se; }

    const std::vector<std::string>& errors() const { return m_errors; }

private:
    std::unordered_map<std::string, std::function<WidgetPtr(const OVMLNode&)>> m_factories;
    std::shared_ptr<StyleEngine> m_style_engine;
    std::vector<std::string> m_errors;

    WidgetPtr create_widget(const OVMLNode& node);
    void apply_attributes(Widget* w, const OVMLNode& node);
    void register_builtin_factories();
};

} // namespace ovui
