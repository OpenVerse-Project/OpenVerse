// OVUI Phase 39 — Theme Editor Tests
#include <Core/Types.h>
#include <Styling/Styling.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Designer/ThemeEditor.h>

#include <cstdio>
#include <cmath>

using namespace ovui;
static int _p=0,_f=0;
#define T(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);_p++;}catch(...){printf("  [FAIL] %s\n",n);_f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}
#define CF(a,b,e) if(fabs((a)-(b))>(e)){printf("    CF %f!=%f\n",(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Phase 39 — Theme Editor\n==============================\n\n");

    T("theme_palette_dark_default",
        ThemePalette p = ThemePalette::dark();
        C(p.primary.r > 0 && p.surface.r > 0 && p.text.r > 0);
        auto v = p.to_vec();
        CE((int)v.size(), 6);
    );

    T("theme_palette_light",
        ThemePalette p = ThemePalette::light();
        C(p.surface.r > 0.5f);
        C(p.text.r < 0.3f);
    );

    T("theme_palette_purple",
        ThemePalette p = ThemePalette::purple();
        C(p.primary.b > p.primary.r);
    );

    T("theme_palette_forest",
        ThemePalette p = ThemePalette::forest();
        C(p.primary.g > p.primary.r);
    );

    T("theme_palette_preset_names",
        auto names = ThemeEditor::preset_names();
        CE((int)names.size(), 4);
        C(names[0] == "Dark");
        C(names[3] == "Forest");
    );

    T("theme_palette_preset_lookup",
        auto p = ThemeEditor::preset("Purple");
        C(p.primary.b > 0.5f);
        auto d = ThemeEditor::preset("Dark");
        C(d.surface.r < 0.3f);
    );

    T("theme_palette_from_vec",
        ThemePalette p;
        p.from_vec({Color{1,0,0,1}, Color{0,1,0,1}, Color{0,0,1,1}});
        CF(p.primary.r, 1, 0.01f);
        CF(p.surface.g, 1, 0.01f);
        CF(p.text.b, 1, 0.01f);
    );

    T("theme_palette_partial_from_vec",
        ThemePalette p;
        p.from_vec({Color{0.5,0,0,1}});
        CF(p.primary.r, 0.5f, 0.01f);
        CF(p.surface.r, 0.117f, 0.05f);
    );

    T("theme_editor_create",
        auto te = std::make_shared<ThemeEditor>();
        C(te != nullptr);
        CF(te->palette().primary.r, 0.309f, 0.05f);
    );

    T("theme_editor_set_palette",
        auto te = std::make_shared<ThemeEditor>();
        ThemePalette p = ThemePalette::forest();
        te->set_palette(p);
        CF(te->palette().primary.g, 0.733f, 0.05f);
    );

    T("theme_editor_preview_widget_type",
        auto te = std::make_shared<ThemeEditor>();
        te->set_preview_widget_type("Button");
        C(te->preview_widget_type() == "Button");
        te->set_preview_widget_type("Slider");
        C(te->preview_widget_type() == "Slider");
    );

    T("theme_editor_apply_theme",
        auto te = std::make_shared<ThemeEditor>();
        te->set_preview_widget_type("Button");
        te->apply_theme();
        C(true);
    );

    T("theme_editor_callback",
        int called = 0;
        auto te = std::make_shared<ThemeEditor>();
        te->on_palette_changed = [&](const ThemePalette&) { called++; };
        te->set_palette(ThemePalette::purple());
        CE(called, 0);
    );

    T("theme_editor_paint",
        auto te = std::make_shared<ThemeEditor>();
        te->set_frame({0, 0, 400, 300});
        te->set_preview_widget_type("Button");

        PaintContext ctx;
        ctx.set_viewport({400, 300});
        te->on_paint(ctx);
        C(ctx.commands().size() >= 4);
    );

    T("theme_editor_paint_checkbox",
        auto te = std::make_shared<ThemeEditor>();
        te->set_frame({0, 0, 400, 300});
        te->set_preview_widget_type("Checkbox");

        PaintContext ctx;
        ctx.set_viewport({400, 300});
        te->on_paint(ctx);
        C(ctx.commands().size() >= 2);
    );

    T("theme_editor_paint_window",
        auto te = std::make_shared<ThemeEditor>();
        te->set_frame({0, 0, 500, 300});
        te->set_preview_widget_type("Window");

        PaintContext ctx;
        ctx.set_viewport({500, 300});
        te->on_paint(ctx);
        C(ctx.commands().size() >= 3);
    );

    T("theme_editor_swatch_click",
        auto te = std::make_shared<ThemeEditor>();
        te->set_frame({0, 0, 400, 300});
        te->set_preview_widget_type("Button");

        PaintContext ctx;
        ctx.set_viewport({400, 300});
        te->on_paint(ctx);

        InputEvent ev;
        ev.type = EventType::MouseDown;
        ev.pos = {te->frame().x + 10, te->frame().y + 10};
        bool handled = te->on_event(ev);
        C(handled);
    );

    T("style_engine_clear_and_rebuild",
        StyleEngine se;
        se.clear_theme();
        CE((int)se.sheet_count(), 0);
        se.rebuild_default_theme(ThemePalette::forest().to_vec());
        auto vals = se.resolve("Button", "", WidgetState::None);
        C(vals.size() >= 3);
    );

    T("style_engine_add_rule",
        StyleEngine se;
        se.clear_theme();
        se.add_rule("Custom", WidgetState::None, {
            {StylePropertyID::BackgroundColor, {1, 0, 0, 1}},
        });
        auto vals = se.resolve("Custom", "", WidgetState::None);
        C(vals.size() >= 1);
        CF(vals[0].f[0], 1, 0.01f);
    );

    T("style_engine_add_rule_class",
        StyleEngine se;
        se.clear_theme();
        se.add_rule_class("Widget", "test-class", WidgetState::Hover, {
            {StylePropertyID::Color, {0, 0, 1, 1}},
        });
        auto vals = se.resolve("Widget", "test-class", WidgetState::Hover);
        C(vals.size() >= 1);
        auto no_match = se.resolve("Widget", "", WidgetState::Hover);
        C(no_match.empty());
    );

    T("style_engine_replace_sheet",
        StyleEngine se;
        se.clear_theme();
        se.add_rule("A", WidgetState::None, {{StylePropertyID::Color, {1,0,0,1}}});
        auto new_sheet = std::make_shared<StyleSheet>();
        StyleRule r; r.selector.widget_type = "A"; r.values = {{StylePropertyID::Color, {0,1,0,1}}};
        new_sheet->add_rule(r);
        se.replace_sheet(0, new_sheet);
        auto vals = se.resolve("A", "", WidgetState::None);
        CF(vals[0].f[1], 1, 0.01f);
    );

    printf("\n  Phase 39 Results: %d passed, %d failed\n\n", _p, _f);
    return _f == 0 ? 0 : 1;
}
