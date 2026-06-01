// OVUI Phases 21-23 — Comprehensive Tests
// TextEngine (SDF/freetype), Animation (spring/timeline/easing), OVML Parser

#include <Core/Types.h>
#include <Reactive/Reactive.h>
#include <Graphics/TextEngine.h>
#include <Animation/Animation.h>
#include <Compiler/OVMLParser.h>

#include <cstdio>
#include <cmath>

using namespace ovui;
static int p=0,f=0;
#define T_TEST(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);p++;}catch(...){printf("  [FAIL] %s\n",n);f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}
#define CF(a,b,e) if(fabs((a)-(b))>(e)){printf("    CF %f!=%f\n",(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Phase 21-23 Tests\n======================\n\n");

    // ---- TextEngine ----
    T_TEST("textengine_create",
        TextEngine te;
        C(true);
    );

    T_TEST("textengine_load_font",
        TextEngine te;
        bool ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        C(ok);
        auto m = te.metrics();
        C(m.ascender > 0);
        C(m.line_height > 0);
    );

    T_TEST("textengine_fallback_load",
        TextEngine te;
        bool ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        C(ok);
    );

    T_TEST("textengine_glyph",
        TextEngine te;
        bool ok = false;
        ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) { printf("    SKIP: no font\n"); break; }
        GlyphInfo g = te.get_glyph('A');
        C(g.codepoint == 'A');
        C(g.advance_x > 0);
    );

    T_TEST("textengine_measure",
        TextEngine te;
        bool ok = false;
        ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) { printf("    SKIP: no font\n"); break; }
        float w = te.measure_text("Hello World");
        C(w > 10);
    );

    T_TEST("textengine_layout",
        TextEngine te;
        bool ok = false;
        ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) { printf("    SKIP: no font\n"); break; }
        TextLayout lo = te.layout_text("Test");
        C(lo.positions.size() == 4);
        C(lo.total_width > 0);
    );

    T_TEST("textengine_set_point_size",
        TextEngine te;
        bool ok = false;
        ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 12.0f);
        if(!ok) ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 12.0f);
        if(!ok) { printf("    SKIP: no font\n"); break; }
        CF(te.point_size(), 12, 0.1);
        te.set_point_size(24);
        CF(te.point_size(), 24, 0.1);
    );

    T_TEST("textengine_atlas",
        TextEngine te;
        bool ok = false;
        ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) ok = te.load_font("/usr/share/fonts/Adwaita/AdwaitaSans-Regular.ttf", 14.0f);
        if(!ok) { printf("    SKIP: no font\n"); break; }
        auto atlas = te.get_atlas();
        C(atlas.width == 1024);
        C(atlas.height == 1024);
    );

    // ---- Animation ----
    T_TEST("easing_linear",
        EasingCurve e(EasingType::Linear);
        CF(e.evaluate(0.0f), 0, 0.001);
        CF(e.evaluate(0.5f), 0.5, 0.001);
        CF(e.evaluate(1.0f), 1, 0.001);
    );

    T_TEST("easing_ease_in_out",
        EasingCurve e(EasingType::EaseInOut);
        CF(e.evaluate(0.0f), 0, 0.001);
        CF(e.evaluate(0.5f), 0.5, 0.001);
        CF(e.evaluate(1.0f), 1, 0.001);
    );

    T_TEST("easing_all_types",
        for(int t=0;t<=(int)EasingType::Custom;t++){EasingCurve e((EasingType)t);float v=e.evaluate(0.3f);C(!std::isnan(v));}
    );

    T_TEST("easing_custom",
        auto e = EasingCurve::custom([](float t){return t*t*t;});
        CF(e.evaluate(0.5f), 0.125, 0.001);
    );

    T_TEST("timeline_create",
        Timeline tl;
        C(!tl.is_playing());
        C(!tl.is_finished());
    );

    T_TEST("timeline_keyframes",
        Timeline tl;
        KeyframeTrack kt{"opacity", {{0,0,{}},{0.5f,1,{}},{1.0f,0,{}}}};
        tl.add_track(kt);
        CF(tl.evaluate("opacity", -1), 0, 0.001);
        tl.play();
        tl.advance(0.5f);
        CF(tl.evaluate("opacity", -1), 1, 0.01);
        tl.advance(0.6f);
        CF(tl.evaluate("opacity", -1), 0, 0.01);
    );

    T_TEST("timeline_duration",
        Timeline tl;
        KeyframeTrack kt{"x", {{0,0,{}},{2.0f,100,{}}}};
        tl.add_track(kt);
        CF(tl.duration(), 2.0, 0.01);
    );

    T_TEST("timeline_loop",
        Timeline tl;
        KeyframeTrack kt{"x", {{0,0,{}},{1.0f,10,{}}}};
        tl.add_track(kt);
        tl.set_loop(true);
        tl.play();
        tl.advance(1.5f);
        C(tl.is_playing());
        CF(tl.current_time(), 0.5, 0.01);
    );

    T_TEST("spring_animation_rest",
        SpringAnimation sp;
        C(sp.is_resting());
        sp.set_target(100);
        C(!sp.is_resting());
    );

    T_TEST("spring_animation_converge",
        SpringAnimation sp;
        sp.configure({200,20,1,0,0.01f});
        sp.set_target(50);
        for(int i=0;i<1000;i++)sp.advance(0.016f);
        CF(sp.current(), 50, 0.1);
        C(sp.is_resting());
    );

    T_TEST("animation_controller",
        AnimationController ac;
        auto& sp = ac.spring("bounce");
        sp.set_target(42);
        auto& tl = ac.timeline("fade");
        KeyframeTrack kt{"opacity", {{0,1,{}},{0.3f,0,{}}}};
        tl.add_track(kt);
        tl.play();
        ac.advance_all(0.16f);
        C(true);
    );

    T_TEST("keyframe_track_sort",
        KeyframeTrack kt{"x", {{1.0f,10,{}},{0,0,{}},{0.5f,5,{}}}};
        kt.sort();
        CF(kt.keyframes[0].time, 0, 0.01);
        CF(kt.keyframes[1].time, 0.5, 0.01);
        CF(kt.keyframes[2].time, 1.0, 0.01);
    );

    // ---- OVML Parser ----
    T_TEST("ovml_parse_simple",
        OVMLParser parser;
        auto root = parser.parse("<Window title=\"Test\"><Button text=\"Click\"/></Window>");
        C(!parser.has_errors());
        C(root->child_count() == 1);
        C(root->children[0]->name == "Window");
        C(root->children[0]->children[0]->name == "Button");
    );

    T_TEST("ovml_parse_attributes",
        OVMLParser parser;
        auto root = parser.parse("<Box width=\"200\" height=\"100\"/>");
        C(!parser.has_errors());
        auto node = root->children[0].get();
        C(node->get_attr("width") == "200");
        C(node->get_attr("height") == "100");
        C(node->get_attr("color", "#FFF") == "#FFF");
    );

    T_TEST("ovml_parse_nested",
        OVMLParser parser;
        auto root = parser.parse(
            "<VBox spacing=\"8\">"
            "  <HBox><Text text=\"A\"/><Text text=\"B\"/></HBox>"
            "  <Button text=\"Submit\"/>"
            "</VBox>"
        );
        C(!parser.has_errors());
        C(root->children[0]->child_count() == 2);
    );

    T_TEST("ovml_parse_self_closing",
        OVMLParser parser;
        auto root = parser.parse("<Image src=\"icon.png\"/><Slider min=\"0\" max=\"100\"/>");
        C(!parser.has_errors());
        C(root->child_count() == 2);
    );

    T_TEST("ovml_validator_known",
        C(OVMLValidator::validate_widget_type("Button"));
        C(OVMLValidator::validate_widget_type("ListView"));
        C(!OVMLValidator::validate_widget_type("XxxBadWidget"));
    );

    T_TEST("ovml_validator_suggest",
        auto sug = OVMLValidator::suggest_widget_type("Botton");
        C(!sug.empty());
    );

    T_TEST("ovml_validator_tree",
        OVMLParser parser;
        auto root = parser.parse("<Window><Buttn text=\"X\"/></Window>");
        std::vector<std::string> warnings;
        bool ok = OVMLValidator::validate(*root, warnings);
        C(!ok);
    );

    T_TEST("ovml_factory_register",
        auto& f = OVMLWidgetFactory::instance();
        f.register_type("CustomWidget",[](const OVMLNode&){return (void*)0x1;});
        C(f.has_type("CustomWidget"));
        C(!f.has_type("NoSuch"));
        C(f.registered_types().size() > 0);
    );

    printf("\n======================\n");
    printf("Results: %d passed, %d failed, %d total\n", p, f, p+f);
    return f;
}
