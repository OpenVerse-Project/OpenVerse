#include "PropertyInspector.h"
#include <algorithm>
#include <cmath>
#include <sstream>
#include <iomanip>

namespace ovui {

PropertyDescriptor PropertyDescriptor::make_float(const std::string& name,
    std::function<float()> get, std::function<void(float)> set,
    float min, float max, float step) {
    PropertyDescriptor d;
    d.name = name;
    d.type = PropertyType::Float;
    d.float_getter = std::move(get);
    d.float_setter = std::move(set);
    d.float_min = min;
    d.float_max = max;
    d.float_step = step;
    return d;
}

PropertyDescriptor PropertyDescriptor::make_int(const std::string& name,
    std::function<int()> get, std::function<void(int)> set,
    int min, int max, int step) {
    PropertyDescriptor d;
    d.name = name;
    d.type = PropertyType::Int;
    d.int_getter = std::move(get);
    d.int_setter = std::move(set);
    d.int_min = min;
    d.int_max = max;
    d.int_step = step;
    return d;
}

PropertyDescriptor PropertyDescriptor::make_bool(const std::string& name,
    std::function<bool()> get, std::function<void(bool)> set) {
    PropertyDescriptor d;
    d.name = name;
    d.type = PropertyType::Bool;
    d.bool_getter = std::move(get);
    d.bool_setter = std::move(set);
    return d;
}

PropertyDescriptor PropertyDescriptor::make_string(const std::string& name,
    std::function<std::string()> get, std::function<void(const std::string&)> set) {
    PropertyDescriptor d;
    d.name = name;
    d.type = PropertyType::String;
    d.string_getter = std::move(get);
    d.string_setter = std::move(set);
    return d;
}

PropertyDescriptor PropertyDescriptor::make_color(const std::string& name,
    std::function<Color()> get, std::function<void(Color)> set) {
    PropertyDescriptor d;
    d.name = name;
    d.type = PropertyType::Color;
    d.color_getter = std::move(get);
    d.color_setter = std::move(set);
    return d;
}

PropertyDescriptor PropertyDescriptor::make_enum(const std::string& name,
    std::function<int()> get, std::function<void(int)> set,
    std::vector<std::string> options) {
    PropertyDescriptor d;
    d.name = name;
    d.type = PropertyType::Enum;
    d.enum_options = std::move(options);
    d.enum_getter = std::move(get);
    d.enum_setter = std::move(set);
    return d;
}

PropertyDescriptor PropertyDescriptor::make_vec2(const std::string& name,
    std::function<float()> get_x, std::function<void(float)> set_x,
    std::function<float()> get_y, std::function<void(float)> set_y) {
    PropertyDescriptor d;
    d.name = name;
    d.type = PropertyType::Vec2;
    d.float_getter = std::move(get_x);
    d.float_setter = std::move(set_x);
    d.float_min = -10000;
    d.float_max = 10000;
    d.float_step = 1;
    d.string_getter = [get_y = std::move(get_y)]() {
        return std::to_string(get_y());
    };
    d.string_setter = [set_y = std::move(set_y)](const std::string& s) {
        try { set_y(std::stof(s)); } catch (...) {}
    };
    return d;
}

PropertyDescriptor PropertyDescriptor::make_category(const std::string& category) {
    PropertyDescriptor d;
    d.name = category;
    d.type = PropertyType::Flags;
    d.category = category;
    return d;
}

PropertyEditor::PropertyEditor() {
    set_widget_type("PropertyEditor");
}

void PropertyEditor::set_descriptor(const PropertyDescriptor& desc) {
    m_desc = desc;
    build_editor();
}

WidgetPtr create_editor_widget(const PropertyDescriptor& desc, float row_h) {
    switch (desc.type) {
        case PropertyType::Float: {
            auto slider = std::make_shared<Slider>();
            slider->set_range(desc.float_min, desc.float_max);
            if (desc.float_getter) slider->set_value(desc.float_getter());
            slider->value_obs().subscribe([desc](float v) {
                if (desc.float_setter) desc.float_setter(v);
                if (desc.on_changed) desc.on_changed();
            });
            return slider;
        }
        case PropertyType::Int: {
            auto spin = std::make_shared<SpinBox>();
            spin->set_range(desc.int_min, desc.int_max);
            if (desc.int_getter) spin->set_value(desc.int_getter());
            spin->value_obs().subscribe([desc](int v) {
                if (desc.int_setter) desc.int_setter(v);
                if (desc.on_changed) desc.on_changed();
            });
            return spin;
        }
        case PropertyType::Bool: {
            auto cb = std::make_shared<Checkbox>();
            if (desc.bool_getter) cb->set_checked(desc.bool_getter());
            cb->on_toggle = [desc](bool v) {
                if (desc.bool_setter) desc.bool_setter(v);
                if (desc.on_changed) desc.on_changed();
            };
            return cb;
        }
        case PropertyType::String: {
            auto ti = std::make_shared<TextInput>();
            if (desc.string_getter) ti->set_text(desc.string_getter());
            ti->text_obs().subscribe([desc](const std::string& v) {
                if (desc.string_setter) desc.string_setter(v);
                if (desc.on_changed) desc.on_changed();
            });
            return ti;
        }
        case PropertyType::Color: {
            auto box = std::make_shared<Box>();
            if (desc.color_getter) box->set_color(desc.color_getter());
            box->set_frame({0, 0, 24, row_h});
            return box;
        }
        case PropertyType::Enum: {
            auto combo = std::make_shared<ComboBox>();
            combo->set_items(desc.enum_options);
            if (desc.enum_getter) combo->set_selected_index(desc.enum_getter());
            combo->on_change = [desc](int v) {
                if (desc.enum_setter) desc.enum_setter(v);
                if (desc.on_changed) desc.on_changed();
            };
            return combo;
        }
        case PropertyType::Vec2:
        case PropertyType::Vec3:
        case PropertyType::Vec4: {
            auto container = std::make_shared<Container>();
            container->layout_node().direction = FlexDirection::Row;
            container->layout_node().gap = 4;

            auto spin_x = std::make_shared<SpinBox>();
            spin_x->set_range(-10000, 10000);
            if (desc.float_getter) spin_x->set_value((int)desc.float_getter());
            spin_x->value_obs().subscribe([desc](int v) {
                if (desc.float_setter) desc.float_setter((float)v);
                if (desc.on_changed) desc.on_changed();
            });
            container->add_child(spin_x);

            auto spin_y = std::make_shared<SpinBox>();
            spin_y->set_range(-10000, 10000);
            if (desc.string_getter) {
                try { spin_y->set_value(std::stoi(desc.string_getter())); } catch (...) {}
            }
            spin_y->value_obs().subscribe([desc](int v) {
                if (desc.string_setter) desc.string_setter(std::to_string(v));
                if (desc.on_changed) desc.on_changed();
            });
            container->add_child(spin_y);

            if (desc.type == PropertyType::Vec3 || desc.type == PropertyType::Vec4) {
                auto spin_z = std::make_shared<SpinBox>();
                spin_z->set_range(-10000, 10000);
                container->add_child(spin_z);
            }
            if (desc.type == PropertyType::Vec4) {
                auto spin_w = std::make_shared<SpinBox>();
                spin_w->set_range(-10000, 10000);
                container->add_child(spin_w);
            }
            return container;
        }
        default:
            return nullptr;
    }
}

void PropertyEditor::build_editor() {
    if (m_editor) {
        remove_all_children();
        m_editor = nullptr;
    }
    m_editor = create_editor_widget(m_desc, m_row_h);
    if (m_editor) add_child(m_editor);
}

void PropertyEditor::refresh() {
    if (!m_editor) return;
    switch (m_desc.type) {
        case PropertyType::Float:
            if (m_desc.float_getter) {
                auto* sl = dynamic_cast<Slider*>(m_editor.get());
                if (sl) sl->set_value(m_desc.float_getter());
            }
            break;
        case PropertyType::Int:
            if (m_desc.int_getter) {
                auto* sp = dynamic_cast<SpinBox*>(m_editor.get());
                if (sp) sp->set_value(m_desc.int_getter());
            }
            break;
        case PropertyType::Bool:
            if (m_desc.bool_getter) {
                auto* cb = dynamic_cast<Checkbox*>(m_editor.get());
                if (cb) cb->set_checked(m_desc.bool_getter());
            }
            break;
        case PropertyType::String:
            if (m_desc.string_getter) {
                auto* ti = dynamic_cast<TextInput*>(m_editor.get());
                if (ti) ti->set_text(m_desc.string_getter());
            }
            break;
        case PropertyType::Color:
            if (m_desc.color_getter) {
                auto* b = dynamic_cast<Box*>(m_editor.get());
                if (b) b->set_color(m_desc.color_getter());
            }
            break;
        case PropertyType::Enum:
            if (m_desc.enum_getter) {
                auto* co = dynamic_cast<ComboBox*>(m_editor.get());
                if (co) co->set_selected_index(m_desc.enum_getter());
            }
            break;
        default: break;
    }
}

PropertyInspector::PropertyInspector() {
    set_widget_type("PropertyInspector");
    layout_node().direction = FlexDirection::Column;
    layout_node().gap = 2;
    layout_node().padding = {4, 4, 4, 4};
}

void PropertyInspector::add_property(const PropertyDescriptor& desc) {
    m_descriptors.push_back(desc);
    m_dirty = true;
}

void PropertyInspector::add_properties(const std::vector<PropertyDescriptor>& descs) {
    for (auto& d : descs) m_descriptors.push_back(d);
    m_dirty = true;
}

void PropertyInspector::clear_properties() {
    m_descriptors.clear();
    m_editors.clear();
    remove_all_children();
    m_dirty = false;
}

void PropertyInspector::invalidate() { m_dirty = true; }

void PropertyInspector::rebuild() {
    m_editors.clear();
    remove_all_children();
    if (m_descriptors.empty()) { m_dirty = false; return; }

    for (auto& desc : m_descriptors) {
        if (desc.hidden) continue;

        if (desc.type == PropertyType::Flags && !desc.category.empty()) {
            auto label = std::make_shared<Text>();
            label->set_text(desc.name);
            label->set_frame({0, 0, 200, std::max(20.0f, m_row_height)});
            add_child(label);
            continue;
        }

        auto editor = std::make_shared<PropertyEditor>();
        editor->set_descriptor(desc);
        editor->set_row_height(m_row_height);
        editor->set_frame({0, 0, 200, m_row_height});

        auto row = std::make_shared<Container>();
        row->layout_node().direction = FlexDirection::Row;
        row->layout_node().gap = 4;

        auto label = std::make_shared<Text>();
        label->set_text(desc.name);
        label->set_frame({0, 0, m_label_width, m_row_height});
        row->add_child(label);
        row->add_child(editor);

        add_child(row);
        m_editors.push_back(editor);
    }
    m_dirty = false;
}

void PropertyInspector::refresh_all() {
    for (auto& e : m_editors) if (e) e->refresh();
}

void PropertyInspector::on_paint(PaintContext& ctx) {
    if (m_dirty) rebuild();
    Container::on_paint(ctx);
}

bool PropertyInspector::on_event(InputEvent& ev) {
    if (m_dirty) rebuild();
    return Container::on_event(ev);
}

} // namespace ovui
