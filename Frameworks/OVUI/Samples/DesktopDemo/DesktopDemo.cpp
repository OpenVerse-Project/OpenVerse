// OVUI Desktop Demo — Window Manager showcase
#include <Core/Types.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Desktop/WindowManager.h>
#include <Designer/ThemeEditor.h>
#include <Graphics/RenderBackend.h>

#ifdef OVUI_HAS_VULKAN
#include <Graphics/Vulkan/VulkanBackend.h>
#endif

#include <cstdio>
#include <cstdlib>
#include <thread>
#include <chrono>
#include <cstring>

using namespace ovui;

int main(int argc, char** argv) {
    printf("\nOVUI Desktop Demo\n=================\n");

    bool use_vulkan = false;
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--vulkan") == 0) use_vulkan = true;
    }

    int vp_w = 1600, vp_h = 960;

    auto workspace = std::make_shared<Workspace>();
    workspace->set_frame({0, 0, (float)vp_w, (float)vp_h});
    workspace->set_menubar_height(28);

    auto wm = workspace->window_manager();

    auto inspector = std::make_shared<ToolWindow>();
    inspector->set_title("Inspector");
    inspector->set_frame({vp_w - 300.0f, 28.0f, 300.0f, (float)vp_h - 28.0f});
    workspace->add_floating_window(inspector);

    auto console = std::make_shared<ToolWindow>();
    console->set_title("Console");
    console->set_frame({0.0f, (float)vp_h - 200.0f, (float)vp_w - 300.0f, 200.0f});
    workspace->add_floating_window(console);

    auto project = std::make_shared<DockWindow>();
    project->set_title("Project");
    project->set_frame({0, 28, 250, (float)vp_h - 28});
    workspace->add_docked_window(project, DockWindow::DockSide::Left, 0.18f);

    auto hierarchy = std::make_shared<DockWindow>();
    hierarchy->set_title("Hierarchy");
    hierarchy->set_frame({0, 0, 250, (float)vp_h - 228});

    auto scene = std::make_shared<EnhancedWindow>();
    scene->set_title("Scene View");
    scene->set_frame({280, 60, 700, 500});
    wm->add_window(scene);
    workspace->add_child(scene);

    auto properties = std::make_shared<EnhancedWindow>();
    properties->set_title("Properties");
    properties->set_frame({280, 580, 700, 250});
    wm->add_window(properties);
    workspace->add_child(properties);

    auto animation = std::make_shared<ToolWindow>();
    animation->set_title("Animation");
    animation->set_frame({0, (float)vp_h - 400, 400, 200});
    workspace->add_floating_window(animation);

    auto dialog = std::make_shared<ModalDialog>();
    dialog->set_title("Settings");
    dialog->set_frame({vp_w * 0.25f, vp_h * 0.25f, vp_w * 0.5f, vp_h * 0.5f});

    auto menu_bar = std::make_shared<MenuBar>();
    menu_bar->set_frame({0, 0, (float)vp_w, 28});
    menu_bar->add_menu("File", {
        {"New", "Ctrl+N", [](){ printf("New\n"); }},
        {"Open", "Ctrl+O", [](){ printf("Open\n"); }},
        {"Save", "Ctrl+S", [](){ printf("Save\n"); }},
        {"", "", nullptr, true},
        {"Exit", "Alt+F4", [](){ printf("Exit\n"); }},
    });
    menu_bar->add_menu("Edit", {
        {"Undo", "Ctrl+Z", [](){}},
        {"Redo", "Ctrl+Y", [](){}},
        {"", "", nullptr, true},
        {"Preferences", "", [&](){
            dialog->show(workspace, {(float)vp_w, (float)vp_h});
        }},
    });
    menu_bar->add_menu("View", {
        {"Fullscreen", "F11", [](){}},
        {"Zoom In", "Ctrl++", [](){}},
        {"Zoom Out", "Ctrl+-", [](){}},
    });
    menu_bar->add_menu("Help", {
        {"About", "", [](){ printf("OVUI Desktop Demo v1.0\n"); }},
    });
    workspace->add_child(menu_bar);

    PaintContext ctx;

    if (use_vulkan) {
#ifdef OVUI_HAS_VULKAN
        register_vulkan_backend();

        Renderer renderer;
        auto vk_be = RenderBackendRegistry::instance().create("vulkan");
        if (vk_be) {
        renderer.set_backend(std::move(vk_be));
        renderer.initialize(vp_w, vp_h);
        auto* vk_ptr = static_cast<VulkanRenderBackend*>(renderer.backend());

        while (vk_ptr->is_running()) {
            vk_ptr->poll_events();
                ctx.reset();
                ctx.set_viewport({(float)vp_w, (float)vp_h});
                workspace->on_paint(ctx);

                auto cmds = convert_paint_commands(ctx);
                renderer.begin_frame();
                renderer.render(cmds);
                renderer.end_frame();
                renderer.present();

                std::this_thread::sleep_for(std::chrono::milliseconds(8));
            }
            renderer.shutdown();
        }
        return 0;
#else
        fprintf(stderr, "Vulkan not available\n");
        return 1;
#endif
    }

    printf("Desktop workspace created with %d windows\n", wm->window_count());
    printf("  Inspector (%s)\n", inspector->title().c_str());
    printf("  Console (%s)\n", console->title().c_str());
    printf("  Project (docked)\n");
    printf("  Scene View\n");
    printf("  Properties\n");
    printf("  Animation\n");
    printf("  MenuBar (File, Edit, View, Help)\n");
    printf("  Modal dialog ready\n");

    workspace->save_layout("/tmp/ovui_layout.txt");
    printf("\nLayout saved to /tmp/ovui_layout.txt\n");
    printf("Layout restored: %s\n",
           workspace->load_layout("/tmp/ovui_layout.txt") ? "yes" : "no");

    wm->set_active_window(scene);
    auto active = wm->active_window();
    printf("Active window: %s\n", active ? active->title().c_str() : "none");

    wm->bring_to_front(inspector);
    printf("Inspector brought to front\n");

    printf("\nDemo complete. Use --vulkan for interactive rendering.\n");
    return 0;
}
