// OVUI Desktop Demo — Window Manager showcase with real UI content
#include <Core/Types.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Desktop/WindowManager.h>
#include <Designer/PropertyInspector.h>
#include <Designer/AnimationTimeline.h>
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

namespace {

void populate_inspector(std::shared_ptr<ToolWindow> win) {
    auto pi = std::make_shared<PropertyInspector>();
    pi->set_frame({0, 24, 300, 600});
    pi->set_label_width(90);
    pi->set_row_height(26);

    float pos_x = 100, pos_y = 50, scale = 1.0f;
    bool visible = true;
    int mode = 0;
    std::string name = "Cube";

    pi->add_property(PropertyDescriptor::make_category("Transform"));
    pi->add_property(PropertyDescriptor::make_vec2("Position",
        [&](){return pos_x;}, [&](float v){pos_x=v;},
        [&](){return pos_y;}, [&](float v){pos_y=v;}));
    pi->add_property(PropertyDescriptor::make_float("Scale",
        [&](){return scale;}, [&](float v){scale=v;}, 0.01f, 10, 0.1f));

    pi->add_property(PropertyDescriptor::make_category("Appearance"));
    pi->add_property(PropertyDescriptor::make_string("Name",
        [&](){return name;}, [&](const std::string& v){name=v;}));
    pi->add_property(PropertyDescriptor::make_bool("Visible",
        [&](){return visible;}, [&](bool v){visible=v;}));
    pi->add_property(PropertyDescriptor::make_enum("Render Mode",
        [&](){return mode;}, [&](int v){mode=v;},
        {"Wireframe", "Solid", "Textured"}));

    win->add_child(pi);
}

void populate_console(std::shared_ptr<ToolWindow> win) {
    auto container = std::make_shared<Container>();
    container->layout_node().direction = FlexDirection::Column;
    container->layout_node().gap = 2;
    container->set_frame({0, 24, 1300, 170});

    auto input_row = std::make_shared<Container>();
    input_row->layout_node().direction = FlexDirection::Row;
    input_row->layout_node().gap = 4;
    input_row->set_frame({0, 0, 1300, 28});

    auto text_input = std::make_shared<TextInput>();
    text_input->set_frame({0, 0, 1200, 28});
    text_input->set_placeholder("> Enter command...");
    input_row->add_child(text_input);

    auto send_btn = std::make_shared<Button>();
    send_btn->set_text("Send");
    send_btn->set_frame({0, 0, 80, 28});
    input_row->add_child(send_btn);

    container->add_child(input_row);

    const char* log_lines[] = {
        "[10:32:01] Engine initialized",
        "[10:32:02] Loading assets...",
        "[10:32:05] 142 assets loaded (1.2s)",
        "[10:32:05] Scene 'MainScene' opened",
        "[10:32:06] Renderer: Vulkan 1.2 ready",
        "[10:32:07] Physics engine: ready",
        "[10:32:08] Audio system: ALSA initialized",
        "[10:32:10] Network: lobby created",
        "[10:32:15] Warning: texture 'brick' missing mipmaps",
        "[10:32:20] Script engine: Lua 5.4 VM loaded",
        "[10:32:30] > hello_ovui()",
    };
    for (auto* line : log_lines) {
        auto txt = std::make_shared<Text>();
        txt->set_text(line);
        txt->set_frame({4, 0, 1300, 18});
        container->add_child(txt);
    }

    win->add_child(container);
}

void populate_project(std::shared_ptr<DockWindow> win) {
    auto panel = std::make_shared<Container>();
    panel->layout_node().direction = FlexDirection::Column;
    panel->set_frame({0, 24, 250, 900});

    auto header = std::make_shared<Container>();
    header->layout_node().direction = FlexDirection::Row;
    header->layout_node().gap = 4;
    header->set_frame({4, 4, 240, 28});

    auto search = std::make_shared<TextInput>();
    search->set_frame({0, 0, 160, 24});
    search->set_placeholder("Search files...");
    header->add_child(search);

    auto new_folder_btn = std::make_shared<Button>();
    new_folder_btn->set_text("+");
    new_folder_btn->set_frame({0, 0, 32, 24});
    header->add_child(new_folder_btn);

    panel->add_child(header);

    const char* files[] = {
        "  src/", "    main.cpp", "    engine.cpp", "    renderer.cpp",
        "  assets/", "    textures/", "      brick.png", "      grass.png",
        "      skybox.hdr", "    models/", "      cube.obj", "      sphere.obj",
        "    shaders/", "      ui_rect.vert", "      ui_rect.frag",
        "  scenes/", "    MainScene.ovscene",
        "  scripts/", "    player.lua", "    enemy.lua",
    };
    for (auto* f : files) {
        auto txt = std::make_shared<Text>();
        txt->set_text(f);
        txt->set_frame({8, 0, 230, 18});
        panel->add_child(txt);
    }

    win->add_child(panel);
}

void populate_scene(std::shared_ptr<EnhancedWindow> win) {
    auto viewport = std::make_shared<Container>();
    viewport->set_frame({0, 30, 700, 460});

    viewport->add_child(std::make_shared<Box>());

    auto toolbar = std::make_shared<Container>();
    toolbar->layout_node().direction = FlexDirection::Row;
    toolbar->layout_node().gap = 4;
    toolbar->set_frame({4, 0, 700, 28});

    auto select_btn = std::make_shared<Button>();
    select_btn->set_text("Select");
    select_btn->set_frame({0, 0, 60, 24});

    auto move_btn = std::make_shared<Button>();
    move_btn->set_text("Move");
    move_btn->set_frame({0, 0, 60, 24});

    auto rotate_btn = std::make_shared<Button>();
    rotate_btn->set_text("Rotate");
    rotate_btn->set_frame({0, 0, 60, 24});

    auto scale_btn = std::make_shared<Button>();
    scale_btn->set_text("Scale");
    scale_btn->set_frame({0, 0, 60, 24});

    toolbar->add_child(select_btn);
    toolbar->add_child(move_btn);
    toolbar->add_child(rotate_btn);
    toolbar->add_child(scale_btn);

    win->add_child(toolbar);
    win->add_child(viewport);
}

void populate_properties(std::shared_ptr<EnhancedWindow> win) {
    auto pi = std::make_shared<PropertyInspector>();
    pi->set_frame({0, 30, 700, 220});
    pi->set_label_width(100);
    pi->set_row_height(28);

    float fov = 60, near_plane = 0.1f, far_plane = 1000;
    bool shadows = true, ssao = true, bloom = false;
    int aa = 2;

    pi->add_property(PropertyDescriptor::make_category("Camera"));
    pi->add_property(PropertyDescriptor::make_float("FOV",
        [&](){return fov;}, [&](float v){fov=v;}, 1, 179));
    pi->add_property(PropertyDescriptor::make_float("Near",
        [&](){return near_plane;}, [&](float v){near_plane=v;}, 0.001f, 100));
    pi->add_property(PropertyDescriptor::make_float("Far",
        [&](){return far_plane;}, [&](float v){far_plane=v;}, 1, 10000));

    pi->add_property(PropertyDescriptor::make_category("Rendering"));
    pi->add_property(PropertyDescriptor::make_bool("Shadows",
        [&](){return shadows;}, [&](bool v){shadows=v;}));
    pi->add_property(PropertyDescriptor::make_bool("SSAO",
        [&](){return ssao;}, [&](bool v){ssao=v;}));
    pi->add_property(PropertyDescriptor::make_bool("Bloom",
        [&](){return bloom;}, [&](bool v){bloom=v;}));
    pi->add_property(PropertyDescriptor::make_enum("Anti-aliasing",
        [&](){return aa;}, [&](int v){aa=v;},
        {"Off", "FXAA", "MSAA 2x", "MSAA 4x", "TAA"}));

    win->add_child(pi);
}

void populate_animation(std::shared_ptr<ToolWindow> win) {
    auto te = std::make_shared<TimelineEditor>();
    te->set_frame({0, 24, 400, 170});

    auto tl = std::make_shared<Timeline>();
    KeyframeTrack tk_pos;
    tk_pos.property = "Position.x";
    tk_pos.keyframes.push_back({0.0f, 0.0f, EasingType::EaseInOut});
    tk_pos.keyframes.push_back({0.3f, 50.0f, EasingType::EaseInOut});
    tk_pos.keyframes.push_back({0.7f, 100.0f, EasingType::EaseInOut});
    tk_pos.keyframes.push_back({1.0f, 150.0f, EasingType::EaseOut});
    tl->add_track(tk_pos);

    KeyframeTrack tk_rot;
    tk_rot.property = "Rotation";
    tk_rot.keyframes.push_back({0.0f, 0.0f, EasingType::Linear});
    tk_rot.keyframes.push_back({0.5f, 180.0f, EasingType::EaseInOut});
    tk_rot.keyframes.push_back({1.0f, 360.0f, EasingType::Linear});
    tl->add_track(tk_rot);

    KeyframeTrack tk_scale;
    tk_scale.property = "Scale";
    tk_scale.keyframes.push_back({0.0f, 1.0f, EasingType::EaseInOut});
    tk_scale.keyframes.push_back({1.0f, 1.5f, EasingType::EaseOut});
    tl->add_track(tk_scale);

    te->set_timeline(tl);
    win->add_child(te);
}

} // anonymous namespace

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

    auto menu_bar = std::make_shared<MenuBar>();
    menu_bar->add_menu("File", {
        {"New", "Ctrl+N", [](){}},
        {"Open", "Ctrl+O", [](){}},
        {"Save", "Ctrl+S", [](){}},
        {"", "", nullptr, true},
        {"Exit", "Alt+F4", [](){}},
    });
    menu_bar->add_menu("Edit", {
        {"Undo", "Ctrl+Z", [](){}},
        {"Redo", "Ctrl+Y", [](){}},
    });
    menu_bar->add_menu("View", {
        {"Fullscreen", "F11", [](){}},
    });
    menu_bar->add_menu("Help", {
        {"About", "", [](){ printf("OVUI Desktop Demo v1.0\n"); }},
    });
    workspace->add_child(menu_bar);

    auto inspector = std::make_shared<ToolWindow>();
    inspector->set_title("Inspector");
    populate_inspector(inspector);
    workspace->add_floating_window(inspector);

    auto console = std::make_shared<ToolWindow>();
    console->set_title("Console");
    populate_console(console);
    workspace->add_floating_window(console);

    auto project = std::make_shared<DockWindow>();
    project->set_title("Project");
    populate_project(project);
    workspace->add_docked_window(project, DockWindow::DockSide::Left, 0.18f);

    auto scene = std::make_shared<EnhancedWindow>();
    scene->set_title("Scene View");
    populate_scene(scene);
    wm->add_window(scene);
    workspace->add_child(scene);

    auto properties = std::make_shared<EnhancedWindow>();
    properties->set_title("Properties");
    populate_properties(properties);
    wm->add_window(properties);
    workspace->add_child(properties);

    auto animation = std::make_shared<ToolWindow>();
    animation->set_title("Animation");
    populate_animation(animation);
    workspace->add_floating_window(animation);

    auto layout_fn = [&](float w, float h) {
        float mh = 28, proj_w = w * 0.18f, insp_w = 300;
        menu_bar->set_frame({0, 0, w, mh});
        project->set_frame({0, mh, proj_w, h - mh});
        inspector->set_frame({w - insp_w, mh, insp_w, 700});
        float scene_w = w - proj_w - insp_w - 4;
        float scene_h = (h - mh - 200) * 0.55f;
        scene->set_frame({proj_w + 2, mh, scene_w, scene_h});
        properties->set_frame({proj_w + 2, mh + scene_h + 2, scene_w, (h - mh - 200) * 0.45f - 2});
        console->set_frame({0, h - 200, w - insp_w, 200});
        animation->set_frame({0, h - 400, 400, 200});

        for (auto& child : scene->children()) {
            auto* c = dynamic_cast<Container*>(child.get());
            if (c && c->children().size() >= 4) {
                child->set_frame({4, 0, scene_w - 8, 28});
            } else {
                child->set_frame({0, 30, scene_w, scene_h - 34});
            }
        }
        for (auto& child : properties->children()) {
            child->set_frame({0, 30, scene_w, (h - mh - 200) * 0.45f - 30});
        }
        for (auto& child : inspector->children()) {
            child->set_frame({0, 24, insp_w, 676});
        }
        for (auto& child : project->children()) {
            child->set_frame({0, 24, proj_w, h - mh - 24});
        }
        for (auto& child : console->children()) {
            Rect cf = child->frame();
            if (cf.height <= 30) child->set_frame({4, 24, w - insp_w - 8, cf.height});
            else child->set_frame({4, 52, w - insp_w - 8, cf.height});
        }
        for (auto& child : animation->children()) {
            child->set_frame({0, 24, 400, 170});
        }
    };

    workspace->on_layout_changed = [&]() {
        float w = workspace->frame().width;
        float h = workspace->frame().height;
        layout_fn(w, h);
    };
    layout_fn((float)vp_w, (float)vp_h);
    workspace->relayout();

    if (use_vulkan) {
#ifdef OVUI_HAS_VULKAN
        register_vulkan_backend();

        Renderer renderer;
        auto vk_be = RenderBackendRegistry::instance().create("vulkan");
        if (vk_be) {
            renderer.set_backend(std::move(vk_be));
            renderer.initialize(vp_w, vp_h);
            auto* vk_ptr = static_cast<VulkanRenderBackend*>(renderer.backend());

            PaintContext ctx;
            while (vk_ptr->is_running()) {
                vk_ptr->poll_events();

                int new_w = vk_ptr->framebuffer_width();
                int new_h = vk_ptr->framebuffer_height();
                if (new_w != vp_w || new_h != vp_h) {
                    vp_w = new_w;
                    vp_h = new_h;
                    workspace->set_frame({0, 0, (float)vp_w, (float)vp_h});
                    workspace->relayout();
                }

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
    printf("  Inspector   — Position, Scale, Name, Visible, Render Mode\n");
    printf("  Console     — Log output + command input\n");
    printf("  Project     — File browser\n");
    printf("  Scene View  — Toolbar (Select/Move/Rotate/Scale)\n");
    printf("  Properties  — Camera (FOV, Near, Far), Rendering flags\n");
    printf("  Animation   — Timeline: Position, Rotation, Scale tracks\n");
    printf("  MenuBar     — File, Edit, View, Help\n");

    printf("\nDemo complete. Use --vulkan for interactive rendering.\n");
    return 0;
}
