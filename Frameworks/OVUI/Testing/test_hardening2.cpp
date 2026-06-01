// OVUI Hardening Tests — Error paths, bounds checking, corrupted input
#include <Core/Types.h>
#include <Compiler/Bytecode.h>
#include <Compiler/BytecodeVM.h>
#include <Graphics/RenderBackend.h>

#include <cstdio>
#include <cstring>
#include <fstream>
#include <cstdio>

using namespace ovui;
static int p=0,f=0;
#define T(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);p++;}catch(...){printf("  [FAIL] %s\n",n);f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}

int main() {
    printf("\nOVUI Hardening Tests — Error Paths & Bounds\n=============================================\n\n");

    // ---- Bytecode load: corrupted file ----
    T("bytecode_load_empty_file",
        {
            std::ofstream f("/tmp/corrupt.ovbc", std::ios::binary);
            f.close();
        }
        auto mod = BytecodeModule::load("/tmp/corrupt.ovbc");
        C(mod == nullptr);
    );

    T("bytecode_load_bad_magic",
        uint8_t data[] = {0xDE,0xAD,0xBE,0xEF, 0x01,0,0,0};
        {
            std::ofstream f("/tmp/badmagic.ovbc", std::ios::binary);
            f.write((char*)data, 8); f.close();
        }
        auto mod = BytecodeModule::load("/tmp/badmagic.ovbc");
        C(mod == nullptr);
    );

    T("bytecode_load_instruction_overflow",
        uint32_t magic = BytecodeModule::MAGIC;
        uint32_t ver = BytecodeModule::VERSION;
        uint32_t huge_ic = 9999999;
        {
            std::ofstream f("/tmp/big.ovbc", std::ios::binary);
            f.write((char*)&magic, 4); f.write((char*)&ver, 4);
            f.write((char*)&huge_ic, 4); f.close();
        }
        auto mod = BytecodeModule::load("/tmp/big.ovbc");
        C(mod == nullptr);
    );

    T("bytecode_load_truncated_string",
        uint32_t magic = BytecodeModule::MAGIC;
        uint32_t ver = BytecodeModule::VERSION;
        uint32_t ic = 0;
        uint32_t cc = 1;
        uint32_t type = 2; // String
        uint32_t slen = 99999999; // impossibly large
        {
            std::ofstream f("/tmp/bigstr.ovbc", std::ios::binary);
            f.write((char*)&magic, 4); f.write((char*)&ver, 4);
            f.write((char*)&ic, 4); f.write((char*)&cc, 4);
            f.write((char*)&type, 4); f.write((char*)&slen, 4); f.close();
        }
        auto mod = BytecodeModule::load("/tmp/bigstr.ovbc");
        C(mod == nullptr);
    );

    T("bytecode_load_invalid_constant_type",
        uint32_t magic = BytecodeModule::MAGIC;
        uint32_t ver = BytecodeModule::VERSION;
        uint32_t ic = 0;
        uint32_t cc = 1;
        uint32_t bad_type = 99;
        {
            std::ofstream f("/tmp/badtype.ovbc", std::ios::binary);
            f.write((char*)&magic, 4); f.write((char*)&ver, 4);
            f.write((char*)&ic, 4); f.write((char*)&cc, 4);
            f.write((char*)&bad_type, 4); f.close();
        }
        auto mod = BytecodeModule::load("/tmp/badtype.ovbc");
        C(mod == nullptr);
    );

    // ---- BytecodeModule: constant limits ----
    T("bytecode_constant_limit",
        BytecodeModule mod;
        for (int i = 0; i < 600000; i++) {
            int32_t id = mod.add_constant_string("x");
            if (id < 0) break;
        }
        C(mod.constants.size() <= BytecodeModule::MAX_CONSTANTS);
    );

    // ---- VM: bounds-checked constant access ----
    T("vm_get_constant_oob",
        BytecodeModule mod;
        mod.add_constant_string("test");
        auto c = mod.get_constant_string(999);
        C(!c.has_value());
        auto c2 = mod.get_constant_string(-1);
        C(!c2.has_value());
    );

    T("vm_get_constant_wrong_type",
        BytecodeModule mod;
        mod.add_constant_int(42);
        auto c = mod.get_constant_string(0);
        C(!c.has_value());
    );

    // ---- VM: safe parsing ----
    T("vm_safe_stof_valid",
        float v = 0;
        C(BytecodeVM::safe_stof("3.14", v));
        C(v > 3.1f && v < 3.2f);
    );

    T("vm_safe_stof_invalid",
        float v = 0;
        C(!BytecodeVM::safe_stof("abc", v));
    );

    T("vm_safe_stod_invalid",
        double v = 0;
        C(!BytecodeVM::safe_stod("", v));
    );

    T("vm_safe_stoi_negative",
        int v = 0;
        C(BytecodeVM::safe_stoi("-42", v));
        CE(v, -42);
    );

    // ---- VM: SetAttribute with invalid widget type ----
    T("vm_bad_widget_type",
        BytecodeModule mod;
        mod.add_constant_string("NoSuchWidget123");
        mod.add_instruction(BytecodeOp::CreateWidget, 0, 1);
        mod.add_instruction(BytecodeOp::End);

        BytecodeVM vm;
        vm.load_module(std::make_shared<BytecodeModule>(mod));
        auto root = vm.execute();
        C(vm.errors().size() > 0);
    );

    // ---- VM: SetText on wrong widget type ----
    T("vm_settext_on_box",
        BytecodeModule mod;
        mod.add_constant_string("Box");
        mod.add_constant_string("Hello");
        mod.add_instruction(BytecodeOp::CreateWidget, 0, 1);
        mod.add_instruction(BytecodeOp::SetText, 1, 1);
        mod.add_instruction(BytecodeOp::End);

        BytecodeVM vm;
        vm.load_module(std::make_shared<BytecodeModule>(mod));
        auto root = vm.execute();
        C(vm.errors().size() > 0);
    );

    // ---- Render backend: out-of-bounds ----
    T("render_rect_outside_framebuffer",
        auto be = std::make_unique<CPURenderBackend>();
        be->initialize();
        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::RectCmd, {-1000,-1000,50,50}, {1,0,0,1}, "", 0});
        be->execute_commands(cmds);
        C(be->framebuffer().size() > 0);
        be->shutdown();
    );

    T("render_negative_size",
        auto be = std::make_unique<CPURenderBackend>();
        be->initialize();
        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::RectCmd, {0,0,-50,-50}, {1,0,0,1}, "", 0});
        be->execute_commands(cmds);
        C(be->framebuffer().size() > 0);
        be->shutdown();
    );

    T("render_zero_radius_circle",
        auto be = std::make_unique<CPURenderBackend>();
        be->initialize();
        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::CircleCmd, {100,100,0,0}, {1,0,0,1}, "", 0});
        be->execute_commands(cmds);
        C(be->framebuffer().size() > 0);
        be->shutdown();
    );

    T("render_framebuffer_far_point",
        auto be = std::make_unique<CPURenderBackend>();
        be->initialize();
        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::RectCmd, {50000,50000,100,100}, {1,0,0,1}, "", 0});
        be->execute_commands(cmds);
        C(true);
        be->shutdown();
    );

    // Cleanup
    std::remove("/tmp/corrupt.ovbc");
    std::remove("/tmp/badmagic.ovbc");
    std::remove("/tmp/big.ovbc");
    std::remove("/tmp/bigstr.ovbc");
    std::remove("/tmp/badtype.ovbc");

    printf("\n=============================================\n");
    printf("Results: %d passed, %d failed, %d total\n", p, f, p+f);
    return f;
}
