#include "OVMLCodeGen.h"
#include <cstdlib>

namespace ovui {

OVMLCodeGen::OVMLCodeGen() {
    m_style_engine = std::make_shared<StyleEngine>();
    register_builtin_factories();
}

void OVMLCodeGen::register_widget_factory(
    const std::string& type,
    std::function<WidgetPtr(const OVMLNode&)> factory)
{
    m_factories[type] = std::move(factory);
}

void OVMLCodeGen::register_builtin_factories() {
    register_widget_factory("Container", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<Container>(); });
    register_widget_factory("VBox", [](const OVMLNode&) -> WidgetPtr {
        auto c = std::make_shared<Container>();
        c->set_direction(FlexDirection::Column);
        return c;
    });
    register_widget_factory("HBox", [](const OVMLNode&) -> WidgetPtr {
        auto c = std::make_shared<Container>();
        c->set_direction(FlexDirection::Row);
        return c;
    });
    register_widget_factory("Box", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<Box>(); });
    register_widget_factory("Text", [](const OVMLNode& node) -> WidgetPtr {
        auto t = std::make_shared<Text>();
        if (node.has_attr("text")) t->set_text(node.get_attr("text"));
        if (!node.text_content.empty()) t->set_text(node.text_content);
        return t;
    });
    register_widget_factory("Button", [](const OVMLNode& node) -> WidgetPtr {
        auto b = std::make_shared<Button>();
        if (node.has_attr("text")) b->set_text(node.get_attr("text"));
        return b;
    });
    register_widget_factory("Checkbox", [](const OVMLNode& node) -> WidgetPtr {
        auto cb = std::make_shared<Checkbox>();
        if (node.has_attr("label")) cb->set_label(node.get_attr("label"));
        if (node.has_attr("checked")) cb->set_checked(node.get_attr("checked") == "true");
        return cb;
    });
    register_widget_factory("RadioButton", [](const OVMLNode& node) -> WidgetPtr {
        auto rb = std::make_shared<RadioButton>();
        if (node.has_attr("label")) rb->set_label(node.get_attr("label"));
        return rb;
    });
    register_widget_factory("Slider", [](const OVMLNode& node) -> WidgetPtr {
        auto s = std::make_shared<Slider>();
        if (node.has_attr("min")) s->set_range(std::stof(node.get_attr("min")), s->value());
        if (node.has_attr("max")) s->set_range(s->value(), std::stof(node.get_attr("max")));
        if (node.has_attr("min") && node.has_attr("max"))
            s->set_range(std::stof(node.get_attr("min")), std::stof(node.get_attr("max")));
        if (node.has_attr("value")) s->set_value(std::stof(node.get_attr("value")));
        return s;
    });
    register_widget_factory("TextInput", [](const OVMLNode& node) -> WidgetPtr {
        auto ti = std::make_shared<TextInput>();
        if (node.has_attr("placeholder")) ti->set_placeholder(node.get_attr("placeholder"));
        return ti;
    });
    register_widget_factory("ScrollArea", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<ScrollArea>(); });
    register_widget_factory("Splitter", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<Splitter>(); });
    register_widget_factory("Panel", [](const OVMLNode& node) -> WidgetPtr {
        auto p = std::make_shared<Panel>();
        if (node.has_attr("title")) p->set_title(node.get_attr("title"));
        return p;
    });
    register_widget_factory("TabView", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<TabView>(); });
    register_widget_factory("Window", [](const OVMLNode& node) -> WidgetPtr {
        auto w = std::make_shared<Window>();
        if (node.has_attr("title")) w->set_title(node.get_attr("title"));
        return w;
    });
    register_widget_factory("MenuBar", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<MenuBar>(); });
    register_widget_factory("ListView", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<ListView>(); });
    register_widget_factory("TreeView", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<TreeView>(); });
    register_widget_factory("TableView", [](const OVMLNode&) -> WidgetPtr { return std::make_shared<TableView>(); });
}

void OVMLCodeGen::apply_attributes(Widget* w, const OVMLNode& node) {
    for (auto& attr : node.attributes) {
        std::string name = attr.name, val = attr.value;

        if (name == "id") w->set_id(std::stoull(val));
        else if (name == "class") w->set_class(val);
        else if (name == "width") w->set_frame({0,0,std::stof(val),w->frame().height});
        else if (name == "height") w->set_frame({0,0,w->frame().width,std::stof(val)});
        else if (name == "visible") w->set_visible(val == "true");
        else if (name == "enabled") w->set_enabled(val == "true");
        else if (name == "padding") {
            EdgeInsets p; p.left = p.right = p.top = p.bottom = std::stof(val);
            if (auto* c = dynamic_cast<Container*>(w)) c->set_padding(p);
        }
        else if (name == "gap") {
            if (auto* c = dynamic_cast<Container*>(w)) c->set_gap(std::stof(val));
        }
        else if (name == "direction") {
            if (auto* c = dynamic_cast<Container*>(w)) {
                if (val == "row") c->set_direction(FlexDirection::Row);
                else c->set_direction(FlexDirection::Column);
            }
        }
    }
}

WidgetPtr OVMLCodeGen::create_widget(const OVMLNode& node) {
    std::string type = node.name;
    if (type.empty()) type = "Container";

    auto it = m_factories.find(type);
    WidgetPtr widget;
    if (it != m_factories.end()) {
        widget = it->second(node);
    } else {
        m_errors.push_back("Unknown widget type: " + type);
        widget = std::make_shared<Container>();
    }

    apply_attributes(widget.get(), node);
    widget->set_widget_type(type);

    return widget;
}

WidgetPtr OVMLCodeGen::generate(const OVMLNode& root) {
    m_errors.clear();

    if (root.children.empty()) return nullptr;

    WidgetPtr result;
    for (auto& child : root.children) {
        WidgetPtr w = create_widget(*child);

        std::function<void(const OVMLNode&, WidgetPtr)> build_children =
            [&](const OVMLNode& n, WidgetPtr parent) {
                for (auto& c : n.children) {
                    WidgetPtr child_w = create_widget(*c);
                    parent->add_child(child_w);
                    build_children(*c, child_w);
                }
            };
        build_children(*child, w);

        if (!result) result = w;
        else result->add_child(w);
    }

    if (result) result->style_engine() = *m_style_engine;

    return result;
}

} // namespace ovui
