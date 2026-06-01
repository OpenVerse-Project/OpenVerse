// OVUI Phase 40 — Window Manager + Workspace Tests
#include <Core/Types.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Desktop/WindowManager.h>

#include <cstdio>
#include <cmath>
#include <fstream>

using namespace ovui;
static int _p=0,_f=0;
#define T(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);_p++;}catch(...){printf("  [FAIL] %s\n",n);_f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}
#define CF(a,b,e) if(fabs((a)-(b))>(e)){printf("    CF %f!=%f\n",(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Phase 40 — Window Manager + Workspace\n============================================\n\n");

    T("wm_create",
        auto wm = std::make_shared<WindowManager>();
        C(wm != nullptr);
        CE(wm->window_count(), 0);
    );

    T("wm_add_window",
        auto wm = std::make_shared<WindowManager>();
        auto win = std::make_shared<EnhancedWindow>();
        wm->add_window(win);
        CE(wm->window_count(), 1);
        C(wm->has_window(win));
    );

    T("wm_remove_window",
        auto wm = std::make_shared<WindowManager>();
        auto win = std::make_shared<EnhancedWindow>();
        wm->add_window(win);
        wm->remove_window(win);
        CE(wm->window_count(), 0);
        C(!wm->has_window(win));
    );

    T("wm_duplicate_add",
        auto wm = std::make_shared<WindowManager>();
        auto win = std::make_shared<EnhancedWindow>();
        wm->add_window(win);
        wm->add_window(win);
        CE(wm->window_count(), 1);
    );

    T("wm_multiple_windows",
        auto wm = std::make_shared<WindowManager>();
        auto w1 = std::make_shared<EnhancedWindow>();
        auto w2 = std::make_shared<ToolWindow>();
        auto w3 = std::make_shared<FloatingWindow>();
        wm->add_window(w1);
        wm->add_window(w2);
        wm->add_window(w3);
        CE(wm->window_count(), 3);
    );

    T("wm_active_window",
        auto wm = std::make_shared<WindowManager>();
        auto win = std::make_shared<EnhancedWindow>();
        wm->add_window(win);
        wm->set_active_window(win);
        auto active = wm->active_window();
        C(active == win);
        C(win->is_active());
    );

    T("wm_active_changes_on_new",
        auto wm = std::make_shared<WindowManager>();
        auto w1 = std::make_shared<EnhancedWindow>();
        auto w2 = std::make_shared<EnhancedWindow>();
        wm->add_window(w1);
        wm->add_window(w2);
        wm->set_active_window(w1);
        C(w1->is_active());
        wm->set_active_window(w2);
        C(w2->is_active());
        C(!w1->is_active());
    );

    T("wm_bring_to_front",
        auto wm = std::make_shared<WindowManager>();
        auto w1 = std::make_shared<EnhancedWindow>();
        auto w2 = std::make_shared<EnhancedWindow>();
        wm->add_window(w1);
        wm->add_window(w2);
        auto ws = wm->windows();
        C(ws.back() == w2);
        wm->bring_to_front(w1);
        ws = wm->windows();
        C(ws.back() == w1);
    );

    T("wm_send_to_back",
        auto wm = std::make_shared<WindowManager>();
        auto w1 = std::make_shared<EnhancedWindow>();
        auto w2 = std::make_shared<EnhancedWindow>();
        wm->add_window(w1);
        wm->add_window(w2);
        wm->send_to_back(w2);
        auto ws = wm->windows();
        C(ws.front() == w2);
    );

    T("wm_floating_windows",
        auto wm = std::make_shared<WindowManager>();
        auto w1 = std::make_shared<EnhancedWindow>();
        auto fw = std::make_shared<FloatingWindow>();
        wm->add_window(w1);
        wm->add_window(fw);
        auto fws = wm->floating_windows();
        CE((int)fws.size(), 1);
        C(fws[0] == fw);
    );

    T("wm_docked_windows",
        auto wm = std::make_shared<WindowManager>();
        auto dw = std::make_shared<DockWindow>();
        auto nw = std::make_shared<EnhancedWindow>();
        wm->add_window(dw);
        wm->add_window(nw);
        auto dws = wm->docked_windows();
        CE((int)dws.size(), 1);
        C(dws[0] == dw);
    );

    T("wm_window_at",
        auto wm = std::make_shared<WindowManager>();
        auto win = std::make_shared<EnhancedWindow>();
        win->set_frame({100, 100, 200, 150});
        wm->add_window(win);
        auto hit = wm->window_at({150, 150});
        C(hit == win);
        auto miss = wm->window_at({0, 0});
        C(miss == nullptr);
    );

    T("wm_next_window_id",
        auto wm = std::make_shared<WindowManager>();
        int id1 = wm->next_window_id();
        int id2 = wm->next_window_id();
        C(id2 > id1);
    );

    T("wm_callbacks",
        int added = 0, removed = 0;
        auto wm = std::make_shared<WindowManager>();
        wm->on_window_added = [&](auto) { added++; };
        wm->on_window_removed = [&](auto) { removed++; };
        auto win = std::make_shared<EnhancedWindow>();
        wm->add_window(win);
        CE(added, 1);
        wm->remove_window(win);
        CE(removed, 1);
    );

    T("enhanced_window_create",
        auto win = std::make_shared<EnhancedWindow>();
        C(win != nullptr);
        C(!win->is_minimized());
        C(!win->is_maximized());
    );

    T("enhanced_window_minimize_restore",
        auto win = std::make_shared<EnhancedWindow>();
        win->set_frame({50, 50, 300, 200});
        win->minimize();
        C(win->is_minimized());
        win->restore();
        C(!win->is_minimized());
    );

    T("enhanced_window_maximize_restore",
        auto win = std::make_shared<EnhancedWindow>();
        win->set_frame({50, 50, 300, 200});
        win->maximize({800, 600});
        C(win->is_maximized());
        win->restore();
        C(!win->is_maximized());
    );

    T("enhanced_window_active_state",
        auto win = std::make_shared<EnhancedWindow>();
        C(!win->is_active());
        win->set_active(true);
        C(win->is_active());
        win->set_active(false);
        C(!win->is_active());
    );

    T("enhanced_window_paint_active",
        auto win = std::make_shared<EnhancedWindow>();
        win->set_title("Test");
        win->set_frame({0, 0, 300, 200});
        win->set_active(true);
        PaintContext ctx;
        ctx.set_viewport({400, 300});
        win->on_paint(ctx);
        C(ctx.commands().size() >= 3);
    );

    T("tool_window_create",
        auto tw = std::make_shared<ToolWindow>();
        tw->set_title("Tools");
        C(tw != nullptr);
    );

    T("tool_window_paint",
        auto tw = std::make_shared<ToolWindow>();
        tw->set_title("Inspector");
        tw->set_frame({0, 0, 250, 400});
        PaintContext ctx;
        ctx.set_viewport({300, 500});
        tw->on_paint(ctx);
        C(ctx.commands().size() >= 2);
    );

    T("floating_window_create",
        auto fw = std::make_shared<FloatingWindow>();
        C(fw != nullptr);
        C(fw->is_always_on_top());
        C(fw->is_dockable());
    );

    T("floating_window_paint",
        auto fw = std::make_shared<FloatingWindow>();
        fw->set_title("Float");
        fw->set_frame({50, 50, 200, 150});
        PaintContext ctx;
        ctx.set_viewport({400, 300});
        fw->on_paint(ctx);
        C(ctx.commands().size() >= 3);
    );

    T("dock_window_create",
        auto dw = std::make_shared<DockWindow>();
        C(dw != nullptr);
        CE((int)dw->dock_side(), (int)DockWindow::DockSide::Fill);
    );

    T("dock_window_sides",
        auto dw = std::make_shared<DockWindow>();
        dw->set_dock_side(DockWindow::DockSide::Left);
        CE((int)dw->dock_side(), (int)DockWindow::DockSide::Left);
        dw->set_dock_side(DockWindow::DockSide::Bottom);
        CE((int)dw->dock_side(), (int)DockWindow::DockSide::Bottom);
    );

    T("dock_window_split_ratio",
        auto dw = std::make_shared<DockWindow>();
        dw->set_split_ratio(0.35f);
        CF(dw->split_ratio(), 0.35f, 0.01f);
        dw->set_split_ratio(0.05f);
        C(dw->split_ratio() >= 0.1f);
    );

    T("modal_dialog_create",
        auto md = std::make_shared<ModalDialog>();
        C(md != nullptr);
        C(md->is_modal());
        C(md->is_blocking());
    );

    T("modal_dialog_paint",
        auto md = std::make_shared<ModalDialog>();
        md->set_title("Confirm");
        md->set_frame({100, 100, 300, 200});
        PaintContext ctx;
        ctx.set_viewport({800, 600});
        md->on_paint(ctx);
        C(ctx.commands().size() >= 2);
    );

    T("workspace_create",
        auto ws = std::make_shared<Workspace>();
        C(ws != nullptr);
        C(ws->window_manager() != nullptr);
    );

    T("workspace_add_floating",
        auto ws = std::make_shared<Workspace>();
        auto fw = std::make_shared<FloatingWindow>();
        fw->set_title("Float");
        ws->add_floating_window(fw);
        CE(ws->window_manager()->window_count(), 1);
    );

    T("workspace_add_docked",
        auto ws = std::make_shared<Workspace>();
        auto dw = std::make_shared<DockWindow>();
        dw->set_title("Dock");
        ws->add_docked_window(dw, DockWindow::DockSide::Left);
        CE(ws->window_manager()->docked_windows().size(), (size_t)1);
    );

    T("workspace_paint",
        auto ws = std::make_shared<Workspace>();
        ws->set_frame({0, 0, 800, 600});
        auto fw = std::make_shared<FloatingWindow>();
        fw->set_title("Float");
        fw->set_frame({50, 50, 200, 150});
        ws->add_floating_window(fw);
        auto dw = std::make_shared<DockWindow>();
        dw->set_title("LeftDock");
        dw->set_frame({0, 0, 200, 600});
        ws->add_docked_window(dw, DockWindow::DockSide::Left, 0.2f);

        PaintContext ctx;
        ctx.set_viewport({800, 600});
        ws->on_paint(ctx);
        C(ctx.commands().size() >= 3);
    );

    T("workspace_save_load_layout",
        auto ws = std::make_shared<Workspace>();
        ws->set_frame({0, 0, 800, 600});
        auto w1 = std::make_shared<EnhancedWindow>();
        w1->set_title("W1");
        w1->set_frame({10, 10, 300, 200});
        ws->add_child(w1);
        ws->window_manager()->add_window(w1);
        auto w2 = std::make_shared<EnhancedWindow>();
        w2->set_title("W2");
        w2->set_frame({350, 20, 400, 300});
        ws->add_child(w2);
        ws->window_manager()->add_window(w2);

        const char* path = "/tmp/ovui_ws_layout_test.txt";
        ws->save_layout(path);
        C(std::ifstream(path).good());
        C(ws->load_layout(path));
    );

    T("serialization_roundtrip",
        auto w1 = std::make_shared<EnhancedWindow>();
        w1->set_title("Test");
        w1->set_frame({100, 200, 300, 400});
        std::vector<std::shared_ptr<EnhancedWindow>> wins = {w1};
        auto data = serialize_window_layout(wins);
        C(!data.empty());

        std::vector<WindowRect> rects;
        std::vector<uint32_t> states;
        deserialize_window_layout(data, rects, states);
        CE((int)rects.size(), 1);
        CF(rects[0].x, 100, 0.1f);
        CF(rects[0].y, 200, 0.1f);
        CF(rects[0].width, 300, 0.1f);
        CF(rects[0].height, 400, 0.1f);
    );

    printf("\n  Phase 40 Results: %d passed, %d failed\n\n", _p, _f);
    return _f == 0 ? 0 : 1;
}
