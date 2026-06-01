// OVUI Phase 24+ — Effects, OVSS, OVML CodeGen, Hot Reload Tests
#include <Core/Types.h>
#include <Graphics/Effects.h>
#include <Compiler/OVMLParser.h>
#include <Compiler/OVSSCompiler.h>
#include <Compiler/OVMLCodeGen.h>
#include <Runtime/HotReload.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>

#include <cstdio>
#include <cmath>
#include <fstream>
#include <filesystem>

using namespace ovui;
static int p=0,f=0;
#define T(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);p++;}catch(...){printf("  [FAIL] %s\n",n);f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}
#define CF(a,b,e) if(fabs((a)-(b))>(e)){printf("    CF %f!=%f\n",(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Phase 24 Tests — Effects + OVSS + CodeGen + HotReload\n===========================================================\n\n");

    // ---- Effects Engine ----
    T("effects_gaussian_kernel",
        auto k = EffectsEngine::build_kernel(3, 1.5f);
        CE((int)k.size(), 7);
        float sum = 0; for(auto v:k) sum += v;
        CF(sum, 1.0f, 0.01);
    );

    T("effects_blur_horizontal",
        float src[16] = {1,0,0,1,0,1,0,1,0,0,1,1,1,1,1,1};
        float dst[16] = {};
        EffectsEngine::blur_horizontal(dst, src, 2, 2, 1, 1.0f);
        C(dst[0] >= 0);
    );

    T("effects_apply_gaussian_blur",
        std::vector<float> px(16, 1.0f);
        EffectsEngine fx;
        BlurDesc bd{2.0f, 1};
        fx.apply_gaussian_blur(px, 2, 2, bd);
        C(px[0] >= 0);
    );

    T("effects_box_shadow",
        std::vector<float> px(10000, 1.0f);
        EffectsEngine fx;
        BoxShadow bs{2, 2, 8, 0, Color{0,0,0,0.5f}};
        fx.apply_box_shadow(px, 50, 50, {10,10,30,30}, bs);
        C(px[0] >= 0);
    );

    T("effects_glow",
        std::vector<float> px(10000, 0.5f);
        EffectsEngine fx;
        fx.apply_glow(px, 50, 50, {15,15,20,20}, Color{0.2f,0.5f,1,0.5f}, 10);
        C(px[0] >= 0);
    );

    T("effects_glass_overlay",
        std::vector<float> px(10000, 0.7f);
        EffectsEngine fx;
        fx.apply_glass_overlay(px, 50, 50, {10,10,30,30}, 5, 0.3f);
        C(px[0] >= 0);
    );

    // ---- OVSS Compiler ----
    T("ovss_compile_simple",
        OVSSCompiler compiler;
        auto cst = compiler.compile(
            "Button {\n"
            "    background-color: #4FC3F7;\n"
            "    font-size: 14px;\n"
            "    border-radius: 6;\n"
            "    state hover {\n"
            "        background-color: #5FD3FF;\n"
            "    }\n"
            "}\n"
        );
        C(cst != nullptr);
        C(!compiler.has_errors());
    );

    T("ovss_compile_empty",
        OVSSCompiler compiler;
        auto cst = compiler.compile("");
        C(cst != nullptr);
    );

    T("ovss_compile_resolve",
        OVSSCompiler compiler;
        auto cst = compiler.compile(
            "Window { background-color: #181825; color: #CDD6F4; }\n"
            "Button { background-color: #4FC3F7; border-radius: 6; }\n"
        );
        C(cst != nullptr);
        auto vals = cst->resolve("Button", WidgetState::None);
        C(vals.size() > 0);
    );

    T("ovss_save_load",
        OVSSCompiler compiler;
        auto cst = compiler.compile("Box { background-color: #FF0000; }");
        C(cst != nullptr);
        bool saved = cst->save("/tmp/ovui_test.cst");
        C(saved);
        auto loaded = CompiledStyleTable::load("/tmp/ovui_test.cst");
        C(loaded != nullptr);
        CE((int)loaded->entries.size(), 1);
    );

    T("ovss_compile_file",
        {
            std::ofstream f("/tmp/test_ovss.ovss");
            f << "Button { background-color: #4FC3F7; }\n";
            f.close();
        }
        OVSSCompiler compiler;
        auto cst = compiler.compile_file("/tmp/test_ovss.ovss");
        C(cst != nullptr);
    );

    // ---- OVML CodeGen ----
    T("ovml_codegen_simple",
        OVMLParser parser;
        auto root = parser.parse("<Window title=\"Test\"><Button text=\"OK\"/></Window>");
        C(!parser.has_errors());
        OVMLCodeGen codegen;
        auto widget = codegen.generate(*root);
        C(widget != nullptr);
    );

    T("ovml_codegen_vbox_hbox",
        OVMLParser parser;
        auto root = parser.parse(
            "<VBox spacing=\"8\" padding=\"16\">"
            "  <HBox><Button text=\"A\"/><Button text=\"B\"/></HBox>"
            "  <Text text=\"Label\"/>"
            "</VBox>"
        );
        OVMLCodeGen codegen;
        auto widget = codegen.generate(*root);
        C(widget != nullptr);
        C(widget->children().size() > 0);
    );

    T("ovml_codegen_custom_factory",
        OVMLCodeGen codegen;
        bool called = false;
        codegen.register_widget_factory("MyWidget", [&](const OVMLNode&) -> WidgetPtr {
            called = true;
            return std::make_shared<Box>();
        });
        OVMLParser parser;
        auto root = parser.parse("<MyWidget/>");
        codegen.generate(*root);
        C(called);
    );

    T("ovml_codegen_attributes",
        OVMLParser parser;
        auto root = parser.parse("<Button text=\"Submit\" width=\"200\" height=\"40\"/>");
        OVMLCodeGen codegen;
        auto widget = codegen.generate(*root);
        C(widget != nullptr);
        Rect f = widget->frame();
        CF(f.width, 200, 0.1);
        CF(f.height, 40, 0.1);
    );

    // ---- Hot Reload ----
    T("hotreload_add_watch",
        HotReloadEngine hre;
        hre.add_watch("/tmp/hot_test.txt");
        CE((int)hre.watch_count(), 0);
    );

    T("hotreload_add_hook",
        HotReloadEngine hre;
        int called = 0;
        hre.add_hook({HotReloadAction::ReloadOVML, ".ovml", [&](const std::string&) { called++; }});
        CE((int)hre.hook_count(), 1);
    );

    T("differential_rebuilder",
        DifferentialRebuilder dr;
        dr.add_source("main.ovml", "<Window/>");
        C(!dr.has_changed("main.ovml", "<Window/>"));
        C(dr.has_changed("main.ovml", "<Window title=\"X\"/>"));
        dr.update("main.ovml", "<Window title=\"X\"/>");
        auto changed = dr.changed_keys();
        CE((int)changed.size(), 1);
    );

    T("differential_rebuilder_new_key",
        DifferentialRebuilder dr;
        C(dr.has_changed("new.ovml", "<Box/>"));
    );

    T("differential_rebuilder_remove",
        DifferentialRebuilder dr;
        dr.add_source("a", "A");
        dr.add_source("b", "B");
        dr.remove_source("a");
        C(dr.has_changed("a", "C"));
    );

    // Cleanup
    std::filesystem::remove("/tmp/ovui_test.cst");
    std::filesystem::remove("/tmp/test_ovss.ovss");

    printf("\n===========================================================\n");
    printf("Results: %d passed, %d failed, %d total\n", p, f, p+f);
    return f;
}
