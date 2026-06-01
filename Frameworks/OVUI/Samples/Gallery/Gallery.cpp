// OVUI Vulkan Gallery — Interactive UI rendering demo
#include <Core/Types.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Designer/DesignSurface.h>
#include <Graphics/RenderBackend.h>

#include <cstdio>
#include <cstdlib>
#include <thread>
#include <chrono>

#ifdef OVUI_HAS_VULKAN
#include <Graphics/Vulkan/VulkanBackend.h>
#endif

using namespace ovui;

int main(int argc, char** argv) {
#ifdef OVUI_HAS_VULKAN
    printf("OVUI Vulkan Gallery\n===================\n");

    register_vulkan_backend();

    Renderer renderer;
    auto vk_be = RenderBackendRegistry::instance().create("vulkan");
    if (!vk_be) {
        fprintf(stderr, "Failed to create Vulkan backend\n");
        return 1;
    }

    renderer.set_backend(std::move(vk_be));
    if (!renderer.initialize(1280, 720)) {
        fprintf(stderr, "Failed to initialize Vulkan renderer\n");
        return 1;
    }

    bool use_designer = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--designer") == 0) use_designer = true;
    }

    auto* vulkan_be = static_cast<VulkanRenderBackend*>(renderer.backend());

    auto root = std::make_shared<Window>();
    root->set_title("OVUI Gallery");

    auto btn = std::make_shared<Button>();
    btn->set_text("Hello Vulkan!");
    btn->set_frame({50, 50, 160, 40});
    btn->on_click = []() { printf("Button clicked!\n"); };

    auto slider = std::make_shared<Slider>();
    slider->set_frame({50, 110, 200, 24});
    slider->set_value(75);

    auto checkbox = std::make_shared<Checkbox>();
    checkbox->set_label("Enable feature");
    checkbox->set_frame({50, 150, 200, 24});
    checkbox->set_checked(true);

    auto txt = std::make_shared<Text>();
    txt->set_text("OVUI Vulkan Gallery — Native GPU Rendering");
    txt->set_frame({50, 190, 400, 24});

    auto spinner = std::make_shared<SpinBox>();
    spinner->set_frame({50, 220, 120, 28});
    spinner->set_value(42);

    auto combo = std::make_shared<ComboBox>();
    combo->set_frame({50, 260, 160, 28});
    combo->set_items({"Option A", "Option B", "Option C"});

    PaintContext ctx;

    auto fps_counter = 0;
    auto last_fps = std::chrono::steady_clock::now();
    int frame_count = 0;

    while (vulkan_be->is_running()) {
        vulkan_be->poll_events();

        ctx.reset();
        ctx.set_viewport({1280, 720});

        root->on_paint(ctx);
        btn->on_paint(ctx);
        slider->on_paint(ctx);
        checkbox->on_paint(ctx);
        txt->on_paint(ctx);
        spinner->on_paint(ctx);
        combo->on_paint(ctx);

        auto cmds = convert_paint_commands(ctx);

        renderer.begin_frame();
        renderer.render(cmds);
        renderer.end_frame();
        renderer.present();

        frame_count++;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_fps).count();
        if (elapsed >= 1000) {
            fps_counter = frame_count;
            frame_count = 0;
            last_fps = now;
            printf("FPS: %d, draw commands: %zu\n", fps_counter, cmds.size());
        }
    }

    renderer.shutdown();
    printf("Gallery closed.\n");
    return 0;
#else
    fprintf(stderr, "Vulkan not available. Rebuild with Vulkan support.\n");
    return 1;
#endif
}
