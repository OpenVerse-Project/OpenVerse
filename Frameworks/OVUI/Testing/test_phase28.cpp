// OVUI Phase 28-30 — Bytecode VM + Render Backend Tests
#include <Core/Types.h>
#include <Compiler/Bytecode.h>
#include <Compiler/BytecodeVM.h>
#include <Compiler/OVMLParser.h>
#include <Graphics/RenderBackend.h>
#include <Widgets/Widgets.h>

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
    printf("\nOVUI Phase 28-30 — Bytecode VM + Render Backend\n==================================================\n\n");

    // ---- Bytecode Module ----
    T("bytecode_create",
        BytecodeModule mod;
        CE((int)mod.size(), 0);
    );

    T("bytecode_add_instruction",
        BytecodeModule mod;
        mod.add_instruction(BytecodeOp::CreateWidget, 0, 1);
        CE((int)mod.size(), 1);
    );

    T("bytecode_add_constants",
        BytecodeModule mod;
        int si = mod.add_constant_string("Button");
        int fi = mod.add_constant_float(3.14f);
        int ii = mod.add_constant_int(42);
        CE(si, 0); CE(fi, 1); CE(ii, 2);
    );

    T("bytecode_save_load",
        BytecodeModule mod;
        mod.add_constant_string("Window");
        mod.add_instruction(BytecodeOp::CreateWidget, 0, 1);
        mod.add_instruction(BytecodeOp::End);
        C(mod.save("/tmp/ovui_test.ovbc"));

        auto loaded = BytecodeModule::load("/tmp/ovui_test.ovbc");
        C(loaded != nullptr);
        CE((int)loaded->instructions.size(), 2);
        CE((int)loaded->constants.size(), 1);
    );

    T("bytecode_dump",
        BytecodeModule mod;
        mod.add_constant_string("Test");
        mod.add_instruction(BytecodeOp::Nop);
        mod.add_instruction(BytecodeOp::End);
        mod.dump();
        C(true);
    );

    // ---- Bytecode Compiler ----
    T("bytecode_compile_ovml",
        BytecodeCompiler compiler;
        auto mod = compiler.compile_ovml("<Window title=\"Test\"><Button text=\"OK\"/></Window>");
        C(mod != nullptr);
        C(mod->size() > 0);
    );

    T("bytecode_compile_complex",
        BytecodeCompiler compiler;
        auto mod = compiler.compile_ovml(
            "<VBox spacing=\"8\" padding=\"16\">"
            "  <HBox><Button text=\"A\"/><Button text=\"B\"/></HBox>"
            "  <Text text=\"Hello World\"/>"
            "</VBox>"
        );
        C(mod != nullptr);
        C(mod->size() >= 4);
    );

    // ---- Bytecode VM ----
    T("vm_execute_basic",
        BytecodeCompiler compiler;
        auto mod = compiler.compile_ovml("<Window title=\"MyApp\"/>");
        C(mod != nullptr);

        BytecodeVM vm;
        auto root = vm.execute_module(*mod);
        C(root != nullptr);
        if (auto* w = dynamic_cast<Window*>(root.get())) {
            C(w->title() == "MyApp");
        }
    );

    T("vm_execute_hierarchy",
        BytecodeCompiler compiler;
        auto mod = compiler.compile_ovml(
            "<VBox><HBox><Button text=\"X\"/><Button text=\"Y\"/></HBox></VBox>"
        );
        BytecodeVM vm;
        auto root = vm.execute_module(*mod);
        C(root != nullptr);
        C(root->children().size() > 0);
    );

    T("vm_register_native",
        BytecodeVM vm;
        bool created = false;
        vm.register_native_widget("Custom", [&](){ created = true; return std::make_shared<Box>(); });
        BytecodeModule mod;
        mod.add_constant_string("Custom");
        mod.add_instruction(BytecodeOp::CreateWidget, 0, 1);
        mod.add_instruction(BytecodeOp::SetRoot, 1);
        mod.add_instruction(BytecodeOp::End);
        vm.load_module(std::make_shared<BytecodeModule>(mod));
        auto root = vm.execute();
        C(created);
    );

    T("vm_event_handler",
        BytecodeVM vm;
        bool clicked = false;
        vm.set_event_handler("onSubmit", [&](Widget*) { clicked = true; });
        BytecodeCompiler compiler;
        auto mod = compiler.compile_ovml("<Button text=\"Go\"/>");
        BytecodeVM vm2;
        auto btn = vm2.execute_module(*mod);
        C(btn != nullptr);
    );

    // ---- Render Backend ----
    T("render_backend_registry",
        auto& reg = RenderBackendRegistry::instance();
        auto names = reg.available_backends();
        C(names.size() >= 1);
    );

    T("render_backend_create",
        auto backend = RenderBackendRegistry::instance().create("cpu");
        C(backend != nullptr);
        C(backend->type() == RenderBackendType::CPU);
    );

    T("cpu_backend_initialize",
        auto backend = std::make_unique<CPURenderBackend>();
        C(backend->initialize());
        auto caps = backend->capabilities();
        C(!caps.gpu_accelerated);
        backend->shutdown();
    );

    T("cpu_backend_draw_commands",
        auto backend = std::make_unique<CPURenderBackend>();
        backend->initialize();

        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::RectCmd, {10,10,100,50}, Color::from_hex(0xFF0000FF), "", 4});
        cmds.push_back({RenderDrawCmd::TextCmd, {20,20,0,0}, {1,1,1,1}, "Hello", 14});

        backend->begin_frame();
        backend->execute_commands(cmds);
        backend->end_frame();

        auto stats = backend->stats();
        CE(stats.draw_calls, 2);
        backend->shutdown();
    );

    T("cpu_backend_framebuffer",
        auto backend = std::make_unique<CPURenderBackend>();
        backend->initialize();
        auto& fb = backend->framebuffer();
        C(fb.size() > 0);
        CE(backend->fb_width(), 800);
        CE(backend->fb_height(), 600);
        backend->shutdown();
    );

    T("renderer_pipeline",
        Renderer renderer;
        C(renderer.initialize(640, 480));

        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::RectCmd, {0,0,640,480}, Color::from_hex(0x1E1E2EFF), "", 0});

        renderer.begin_frame();
        renderer.render(cmds);
        renderer.end_frame();

        auto stats = renderer.stats();
        CE(stats.draw_calls, 1);
        renderer.shutdown();
    );

    T("renderer_resize",
        Renderer renderer;
        renderer.initialize(100, 100);
        renderer.resize(200, 150);
        renderer.shutdown();
        C(true);
    );

    T("cpu_backend_circle",
        auto backend = std::make_unique<CPURenderBackend>();
        backend->initialize();
        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::CircleCmd, {40,40,20,20}, {1,0,0,1}, "", 10});
        backend->execute_commands(cmds);
        backend->shutdown();
    );

    // Cleanup
    std::filesystem::remove("/tmp/ovui_test.ovbc");

    printf("\n==================================================\n");
    printf("Results: %d passed, %d failed, %d total\n", p, f, p+f);
    return f;
}
