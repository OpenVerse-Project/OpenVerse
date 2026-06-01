// OVUI Rendering Tests — CPU backend + PPM output
#include <Core/Types.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Graphics/RenderBackend.h>

#include <cstdio>
#include <cmath>
#include <filesystem>

using namespace ovui;
static int p=0,f=0;
#define T(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);p++;}catch(...){printf("  [FAIL] %s\n",n);f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}

static uint32_t sample_fb(const std::vector<uint32_t>& fb, int w, int h, int x, int y) {
    if (x < 0 || x >= w || y < 0 || y >= h) return 0;
    return fb[(size_t)y * (size_t)w + (size_t)x];
}

int main() {
    printf("\nOVUI Rendering Tests — CPU Backend + PPM\n==========================================\n\n");

    T("renderer_create_cpu",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = std::make_unique<CPURenderBackend>();
        renderer->set_backend(std::move(cpu));
        C(renderer->initialize(400, 300));
        renderer->shutdown();
        C(true);
    );

    T("renderer_draw_rect",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(200, 100);

        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::RectCmd, {10, 10, 50, 30}, Color{1,0,0,1}});
        renderer->begin_frame();
        renderer->render(cmds);
        renderer->end_frame();

        auto& fb = cpu->framebuffer();
        int w = cpu->fb_width(), h = cpu->fb_height();
        CE(w, 200);
        CE(h, 100);

        uint32_t center = sample_fb(fb, w, h, 35, 25);
        uint32_t outside = sample_fb(fb, w, h, 5, 5);
        C((center & 0xFF) > 200);
        C(outside != center);
    );

    T("renderer_draw_border",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(200, 100);

        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::BorderCmd, {10, 10, 50, 30}, Color{0,1,0,1}, "", 0, 3});
        renderer->begin_frame();
        renderer->render(cmds);
        renderer->end_frame();

        auto& fb = cpu->framebuffer();
        int w = cpu->fb_width(), h = cpu->fb_height();

        uint32_t top_edge = sample_fb(fb, w, h, 35, 11);
        uint32_t inside = sample_fb(fb, w, h, 35, 25);
        uint32_t outside = sample_fb(fb, w, h, 5, 5);
        C((top_edge >> 8) & 0xFF);
        C(((inside >> 8) & 0xFF) < 200);
        C(outside != top_edge);
    );

    T("renderer_draw_line",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(200, 100);

        std::vector<RenderDrawCmd> cmds;
        RenderDrawCmd lc{RenderDrawCmd::LineCmd};
        lc.p1 = {10, 10};
        lc.p2 = {100, 80};
        lc.color = Color{0,0,1,1};
        lc.border_width = 2;
        cmds.push_back(lc);
        renderer->begin_frame();
        renderer->render(cmds);
        renderer->end_frame();

        auto& fb = cpu->framebuffer();
        int w = cpu->fb_width(), h = cpu->fb_height();

        uint32_t on_line = sample_fb(fb, w, h, 55, 45);
        uint32_t off_line = sample_fb(fb, w, h, 5, 5);
        C(on_line != off_line);
    );

    T("renderer_draw_circle",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(200, 100);

        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::CircleCmd, {50, 10, 40, 40}, Color{1,1,0,1}});
        renderer->begin_frame();
        renderer->render(cmds);
        renderer->end_frame();

        auto& fb = cpu->framebuffer();
        int w = cpu->fb_width(), h = cpu->fb_height();

        uint32_t center = sample_fb(fb, w, h, 70, 30);
        uint32_t outside = sample_fb(fb, w, h, 5, 5);
        C(center != outside);
    );

    T("renderer_draw_text",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(200, 50);

        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::TextCmd, {10, 10, 0, 0}, Color{1,1,1,1}, "Test", 14});
        renderer->begin_frame();
        renderer->render(cmds);
        renderer->end_frame();

        auto& fb = cpu->framebuffer();
        int w = cpu->fb_width(), h = cpu->fb_height();

        uint32_t text_pixel = sample_fb(fb, w, h, 12, 12);
        uint32_t blank = sample_fb(fb, w, h, 5, 5);
        C(text_pixel != blank);
    );

    T("renderer_widget_paint_and_render",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(400, 300);

        auto btn = std::make_shared<Button>();
        btn->set_text("Click Me");
        btn->set_frame({50, 40, 120, 36});

        auto slider = std::make_shared<Slider>();
        slider->set_frame({50, 100, 200, 24});

        PaintContext ctx;
        ctx.set_viewport({400, 300});
        btn->on_paint(ctx);
        slider->on_paint(ctx);

        auto rcmds = convert_paint_commands(ctx);
        C(rcmds.size() >= 4);

        renderer->begin_frame();
        renderer->render(rcmds);
        renderer->end_frame();

        auto& fb = cpu->framebuffer();
        int w = cpu->fb_width(), h = cpu->fb_height();

        uint32_t bg = sample_fb(fb, w, h, 5, 5);
        uint32_t btn_px = sample_fb(fb, w, h, 100, 55);
        C(btn_px != bg);
    );

    T("renderer_widget_panel_paint",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(400, 400);

        auto window = std::make_shared<Window>();
        window->set_title("Test Window");
        window->set_frame({20, 20, 300, 200});

        auto btn = std::make_shared<Button>();
        btn->set_text("OK");
        btn->set_frame({100, 60, 100, 30});
        window->add_child(btn);

        PaintContext ctx;
        ctx.set_viewport({400, 400});
        window->on_paint(ctx);

        auto rcmds = convert_paint_commands(ctx);
        C(rcmds.size() >= 1);

        renderer->begin_frame();
        renderer->render(rcmds);
        renderer->end_frame();

        C(true);
    );

    T("renderer_resize",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(200, 100);
        CE(cpu->fb_width(), 200);
        CE(cpu->fb_height(), 100);
        renderer->resize(800, 600);
        CE(cpu->fb_width(), 800);
        CE(cpu->fb_height(), 600);
    );

    T("renderer_capabilities",
        auto cpu = std::make_unique<CPURenderBackend>();
        auto caps = cpu->capabilities();
        C(!caps.gpu_accelerated);
        C(!caps.bindless_textures);
        CE(caps.max_texture_size, 8192);
    );

    T("renderer_ppm_save",
        auto renderer = std::make_shared<Renderer>();
        auto cpu = new CPURenderBackend();
        renderer->set_backend(std::unique_ptr<CPURenderBackend>(cpu));
        renderer->initialize(100, 50);

        std::vector<RenderDrawCmd> cmds;
        cmds.push_back({RenderDrawCmd::RectCmd, {10, 10, 30, 20}, Color{1,0,0,1}});
        renderer->begin_frame();
        renderer->render(cmds);
        renderer->end_frame();

        const char* ppm_path = "/tmp/ovui_render_test.ppm";
        C(cpu->save_ppm(ppm_path));
        C(std::filesystem::exists(ppm_path));
        auto sz = std::filesystem::file_size(ppm_path);
        C(sz > 100);
    );

    T("renderer_registry_cpu",
        auto& reg = RenderBackendRegistry::instance();
        auto be = reg.create("cpu");
        C(be != nullptr);
        C(be->type() == RenderBackendType::CPU);
    );

    T("renderer_registry_default",
        auto be = RenderBackendRegistry::instance().create_default();
        C(be != nullptr);
    );

    T("renderer_backends_list",
        auto names = RenderBackendRegistry::instance().available_backends();
        C(names.size() >= 1);
    );

    printf("\n  Rendering Results: %d passed, %d failed\n\n", p, f);
    return f == 0 ? 0 : 1;
}
