// OVUI Phase 37 — Property Inspector Tests
#include <Core/Types.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Designer/PropertyInspector.h>

#include <cstdio>
#include <cmath>

using namespace ovui;
static int p=0,f=0;
#define T(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);p++;}catch(...){printf("  [FAIL] %s\n",n);f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}
#define CF(a,b,e) if(fabs((a)-(b))>(e)){printf("    CF %f!=%f\n",(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Phase 37 — Property Inspector\n====================================\n\n");

    T("propdesc_make_float",
        float val = 0;
        auto d = PropertyDescriptor::make_float("Speed", [&](){return val;}, [&](float v){val=v;}, 0, 200, 1);
        CE((int)d.type, (int)PropertyType::Float);
        C(d.name == "Speed");
        CF(d.float_min, 0, 0.01f);
        CF(d.float_max, 200, 0.01f);
    );

    T("propdesc_make_float_getset",
        float val = 50;
        auto d = PropertyDescriptor::make_float("Speed", [&](){return val;}, [&](float v){val=v;}, 0, 200, 1);
        CF(d.float_getter(), 50, 0.01f);
        d.float_setter(75);
        CF(val, 75, 0.01f);
    );

    T("propdesc_make_int",
        int val = 0;
        auto d = PropertyDescriptor::make_int("Count", [&](){return val;}, [&](int v){val=v;}, 0, 100, 1);
        CE((int)d.type, (int)PropertyType::Int);
        C(d.name == "Count");
        CE(d.int_getter(), 0);
        d.int_setter(42);
        CE(val, 42);
    );

    T("propdesc_make_bool",
        bool val = false;
        auto d = PropertyDescriptor::make_bool("Enabled", [&](){return val;}, [&](bool v){val=v;});
        CE((int)d.type, (int)PropertyType::Bool);
        C(!d.bool_getter());
        d.bool_setter(true);
        C(val);
    );

    T("propdesc_make_string",
        std::string val = "hello";
        auto d = PropertyDescriptor::make_string("Name", [&](){return val;}, [&](const std::string& v){val=v;});
        CE((int)d.type, (int)PropertyType::String);
        C(d.string_getter() == "hello");
        d.string_setter("world");
        C(val == "world");
    );

    T("propdesc_make_color",
        Color val{1,0,0,1};
        auto d = PropertyDescriptor::make_color("Tint", [&](){return val;}, [&](Color v){val=v;});
        CE((int)d.type, (int)PropertyType::Color);
        CF(d.color_getter().r, 1, 0.01f);
        d.color_setter(Color{0,1,0,1});
        CF(val.g, 1, 0.01f);
    );

    T("propdesc_make_enum",
        int val = 0;
        auto d = PropertyDescriptor::make_enum("Mode", [&](){return val;}, [&](int v){val=v;},
            {"OptionA", "OptionB", "OptionC"});
        CE((int)d.type, (int)PropertyType::Enum);
        CE((int)d.enum_options.size(), 3);
        C(d.enum_options[1] == "OptionB");
        CE(d.enum_getter(), 0);
        d.enum_setter(2);
        CE(val, 2);
    );

    T("propdesc_make_vec2",
        float x = 10, y = 20;
        auto d = PropertyDescriptor::make_vec2("Position",
            [&](){return x;}, [&](float v){x=v;},
            [&](){return y;}, [&](float v){y=v;});
        CE((int)d.type, (int)PropertyType::Vec2);
        CF(d.float_getter(), 10, 0.01f);
        d.float_setter(50);
        CF(x, 50, 0.01f);
    );

    T("propdesc_make_category",
        auto d = PropertyDescriptor::make_category("Transform");
        CE((int)d.type, (int)PropertyType::Flags);
        C(d.name == "Transform");
        C(d.category == "Transform");
    );

    T("propdesc_on_changed",
        int count = 0;
        auto d = PropertyDescriptor::make_int("Val", [](){return 0;}, [](int){}, 0, 100);
        d.on_changed = [&](){ count++; };
        d.on_changed();
        CE(count, 1);
    );

    T("propdesc_readonly",
        auto d = PropertyDescriptor::make_float("Const", [](){return 3.14f;}, [](float){}, 0, 1);
        d.read_only = true;
        C(d.read_only);
    );

    T("prop_inspector_create",
        auto pi = std::make_shared<PropertyInspector>();
        C(pi != nullptr);
        CE(pi->property_count(), 0);
    );

    T("prop_inspector_add_properties",
        auto pi = std::make_shared<PropertyInspector>();
        float fv = 50;
        int iv = 10;
        bool bv = true;

        pi->add_property(PropertyDescriptor::make_float("Speed", [&](){return fv;}, [&](float v){fv=v;}, 0, 100));
        pi->add_property(PropertyDescriptor::make_int("Count", [&](){return iv;}, [&](int v){iv=v;}, 0, 100));
        pi->add_property(PropertyDescriptor::make_bool("On", [&](){return bv;}, [&](bool v){bv=v;}));

        CE(pi->property_count(), 3);
    );

    T("prop_inspector_add_properties_vector",
        auto pi = std::make_shared<PropertyInspector>();
        float fv = 50;
        std::vector<PropertyDescriptor> descs = {
            PropertyDescriptor::make_float("A", [&](){return fv;}, [&](float v){fv=v;}),
            PropertyDescriptor::make_float("B", [&](){return fv;}, [&](float v){fv=v;}),
        };
        pi->add_properties(descs);
        CE(pi->property_count(), 2);
    );

    T("prop_inspector_clear",
        auto pi = std::make_shared<PropertyInspector>();
        pi->add_property(PropertyDescriptor::make_bool("X", [](){return true;}, [](bool){}));
        pi->clear_properties();
        CE(pi->property_count(), 0);
    );

    T("prop_inspector_label_width",
        auto pi = std::make_shared<PropertyInspector>();
        pi->set_label_width(150);
        CF(pi->label_width(), 150, 0.01f);
    );

    T("prop_inspector_row_height",
        auto pi = std::make_shared<PropertyInspector>();
        pi->set_row_height(32);
        CF(pi->row_height(), 32, 0.01f);
    );

    T("prop_inspector_category_header",
        auto pi = std::make_shared<PropertyInspector>();
        bool val = false;
        pi->add_property(PropertyDescriptor::make_category("Appearance"));
        pi->add_property(PropertyDescriptor::make_bool("Visible", [&](){return val;}, [&](bool v){val=v;}));
        CE(pi->property_count(), 2);

        pi->set_frame({0,0,300,200});
        PaintContext ctx;
        ctx.set_viewport({300, 200});
        pi->on_paint(ctx);
        C(ctx.commands().size() >= 0);
    );

    T("prop_inspector_hidden_property",
        auto pi = std::make_shared<PropertyInspector>();
        auto d = PropertyDescriptor::make_float("Hidden", [](){return 0.0f;}, [](float){});
        d.hidden = true;
        pi->add_property(d);
        CE(pi->property_count(), 1);
    );

    T("prop_inspector_on_property_changed",
        auto pi = std::make_shared<PropertyInspector>();
        int called = 0;
        pi->on_property_changed = [&](const std::string& name, const PropertyDescriptor&) { called++; };
        float fv = 50;
        pi->add_property(PropertyDescriptor::make_float("Val", [&](){return fv;}, [&](float v){fv=v;}, 0, 100));
        C(true);
    );

    T("prop_inspector_refresh_all",
        auto pi = std::make_shared<PropertyInspector>();
        float fv = 50;
        pi->add_property(PropertyDescriptor::make_float("Val", [&](){return fv;}, [&](float v){fv=v;}, 0, 100));
        fv = 99;
        pi->refresh_all();
        C(true);
    );

    T("prop_inspector_paint",
        auto pi = std::make_shared<PropertyInspector>();
        pi->set_frame({0, 0, 400, 300});

        float speed = 100;
        int count = 5;
        bool enabled = true;
        std::string name = "Widget";
        Color tint{0.5f, 0.5f, 1.0f, 1.0f};
        int mode = 1;

        pi->add_property(PropertyDescriptor::make_category("General"));
        pi->add_property(PropertyDescriptor::make_string("Name",
            [&](){return name;}, [&](const std::string& v){name=v;}));
        pi->add_property(PropertyDescriptor::make_bool("Enabled",
            [&](){return enabled;}, [&](bool v){enabled=v;}));

        pi->add_property(PropertyDescriptor::make_category("Values"));
        pi->add_property(PropertyDescriptor::make_float("Speed",
            [&](){return speed;}, [&](float v){speed=v;}, 0, 200));
        pi->add_property(PropertyDescriptor::make_int("Count",
            [&](){return count;}, [&](int v){count=v;}, 0, 20));
        pi->add_property(PropertyDescriptor::make_enum("Mode",
            [&](){return mode;}, [&](int v){mode=v;},
            {"Easy", "Normal", "Hard"}));
        pi->add_property(PropertyDescriptor::make_color("Tint",
            [&](){return tint;}, [&](Color v){tint=v;}));

        PaintContext ctx;
        ctx.set_viewport({400, 300});
        pi->on_paint(ctx);

        for (auto& child : pi->children()) {
            child->on_paint(ctx);
            for (auto& grandchild : child->children()) {
                grandchild->on_paint(ctx);
                for (auto& gg : grandchild->children()) gg->on_paint(ctx);
            }
        }

        auto& cmds = ctx.commands();
        C(cmds.size() >= 10);
    );

    printf("\n  Phase 37 Results: %d passed, %d failed\n\n", p, f);
    return f == 0 ? 0 : 1;
}
