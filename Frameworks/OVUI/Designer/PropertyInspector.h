#pragma once

#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Reactive/Reactive.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <type_traits>

namespace ovui {

enum class PropertyType : uint8_t {
    Float, Int, Bool, String, Color,
    Vec2, Vec3, Vec4,
    Enum, Flags
};

struct PropertyDescriptor {
    std::string name;
    PropertyType type = PropertyType::String;
    std::string category;

    std::function<float()> float_getter;
    std::function<void(float)> float_setter;
    float float_min = 0, float_max = 100;
    float float_step = 1;

    std::function<int()> int_getter;
    std::function<void(int)> int_setter;
    int int_min = 0, int_max = 100;
    int int_step = 1;

    std::function<bool()> bool_getter;
    std::function<void(bool)> bool_setter;

    std::function<std::string()> string_getter;
    std::function<void(const std::string&)> string_setter;

    std::function<Color()> color_getter;
    std::function<void(Color)> color_setter;

    std::vector<std::string> enum_options;
    std::function<int()> enum_getter;
    std::function<void(int)> enum_setter;

    std::function<void()> on_changed;

    bool read_only = false;
    bool hidden = false;

    static PropertyDescriptor make_float(const std::string& name,
        std::function<float()> get, std::function<void(float)> set,
        float min = 0, float max = 100, float step = 1);

    static PropertyDescriptor make_int(const std::string& name,
        std::function<int()> get, std::function<void(int)> set,
        int min = 0, int max = 100, int step = 1);

    static PropertyDescriptor make_bool(const std::string& name,
        std::function<bool()> get, std::function<void(bool)> set);

    static PropertyDescriptor make_string(const std::string& name,
        std::function<std::string()> get, std::function<void(const std::string&)> set);

    static PropertyDescriptor make_color(const std::string& name,
        std::function<Color()> get, std::function<void(Color)> set);

    static PropertyDescriptor make_enum(const std::string& name,
        std::function<int()> get, std::function<void(int)> set,
        std::vector<std::string> options);

    static PropertyDescriptor make_vec2(const std::string& name,
        std::function<float()> get_x, std::function<void(float)> set_x,
        std::function<float()> get_y, std::function<void(float)> set_y);

    static PropertyDescriptor make_category(const std::string& category);
};

class PropertyEditor : public Widget {
public:
    PropertyEditor();
    void set_descriptor(const PropertyDescriptor& desc);
    void refresh();
    const PropertyDescriptor& descriptor() const { return m_desc; }
    void set_row_height(float h) { m_row_h = h; }
private:
    PropertyDescriptor m_desc;
    WidgetPtr m_editor;
    float m_row_h = 24;
    float m_label_w = 100;
    void build_editor();
};

class PropertyInspector : public Container {
public:
    PropertyInspector();

    void add_property(const PropertyDescriptor& desc);
    void add_properties(const std::vector<PropertyDescriptor>& descs);
    void clear_properties();

    void set_label_width(float w) { m_label_width = w; invalidate(); }
    float label_width() const { return m_label_width; }
    void set_row_height(float h) { m_row_height = h; invalidate(); }
    float row_height() const { return m_row_height; }

    void invalidate();
    int property_count() const { return (int)m_descriptors.size(); }

    void refresh_all();

    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;

    void rebuild();

    std::function<void(const std::string&, const PropertyDescriptor&)> on_property_changed;

private:
    std::vector<PropertyDescriptor> m_descriptors;
    std::vector<std::shared_ptr<PropertyEditor>> m_editors;
    float m_label_width = 100;
    float m_row_height = 28;
    bool m_dirty = true;

    void rebuild_internal();
};

} // namespace ovui
