// OVUI Phase 36 — Design Surface Tests
#include <Core/Types.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Designer/DesignSurface.h>

#include <cstdio>
#include <cmath>

using namespace ovui;
static int p=0,f=0;
#define T(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);p++;}catch(...){printf("  [FAIL] %s\n",n);f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}
#define CF(a,b,e) if(fabs((a)-(b))>(e)){printf("    CF %f!=%f\n",(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Phase 36 — Design Surface\n===============================\n\n");

    T("designsurface_create",
        auto ds = std::make_shared<DesignSurface>();
        C(ds != nullptr);
        CE(ds->selection().size(), (size_t)0);
        C(ds->selected() == nullptr);
    );

    T("designsurface_add_widget",
        auto ds = std::make_shared<DesignSurface>();
        auto box = std::make_shared<Box>();
        box->set_frame({10, 20, 100, 80});
        ds->add_child(box);
        CE((int)ds->children().size(), 1);
    );

    T("designsurface_select_widget",
        auto ds = std::make_shared<DesignSurface>();
        auto box = std::make_shared<Box>();
        box->set_frame({50, 50, 100, 80});
        ds->add_child(box);
        ds->select(box);
        C(ds->selected() == box);
        CE(ds->selection().size(), (size_t)1);
        C(ds->is_selected(box));
    );

    T("designsurface_deselect_all",
        auto ds = std::make_shared<DesignSurface>();
        auto box = std::make_shared<Box>();
        ds->add_child(box);
        ds->select(box);
        ds->deselect_all();
        C(ds->selected() == nullptr);
        CE(ds->selection().size(), (size_t)0);
    );

    T("designsurface_multi_select",
        auto ds = std::make_shared<DesignSurface>();
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        b1->set_frame({10, 10, 50, 50});
        b2->set_frame({70, 70, 50, 50});
        ds->add_child(b1);
        ds->add_child(b2);
        ds->select(b1);
        ds->add_to_selection(b2);
        CE(ds->selection().size(), (size_t)2);
        C(ds->is_selected(b1));
        C(ds->is_selected(b2));
    );

    T("designsurface_remove_from_selection",
        auto ds = std::make_shared<DesignSurface>();
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        b1->set_frame({10, 10, 50, 50});
        b2->set_frame({70, 70, 50, 50});
        ds->add_child(b1);
        ds->add_child(b2);
        ds->select(b1);
        ds->add_to_selection(b2);
        ds->remove_from_selection(b1);
        CE(ds->selection().size(), (size_t)1);
        C(ds->selected() == b2);
    );

    T("designsurface_grid_defaults",
        auto ds = std::make_shared<DesignSurface>();
        CF(ds->grid_size(), 8, 0.1f);
        C(ds->grid_enabled());
        C(ds->snap_enabled());
    );

    T("designsurface_grid_set_size",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_grid_size(16);
        CF(ds->grid_size(), 16, 0.1f);
    );

    T("designsurface_grid_clamp_min",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_grid_size(0);
        C(ds->grid_size() >= 1.0f);
    );

    T("designsurface_zoom_default",
        auto ds = std::make_shared<DesignSurface>();
        CF(ds->zoom(), 1.0f, 0.01f);
    );

    T("designsurface_zoom_range",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_zoom(5.0f);
        CF(ds->zoom(), 5.0f, 0.01f);
        ds->set_zoom(0.05f);
        CF(ds->zoom(), 0.05f, 0.01f);
        ds->set_zoom(20.0f);
        C(ds->zoom() <= 10.0f);
        ds->set_zoom(-1.0f);
        C(ds->zoom() >= 0.05f);
    );

    T("designsurface_factory_register",
        auto ds = std::make_shared<DesignSurface>();
        ds->register_widget_type("Button", [](){ return std::make_shared<Button>(); });
        C(ds->has_widget_type("Button"));
        C(!ds->has_widget_type("Slider"));
    );

    T("designsurface_factory_create",
        auto ds = std::make_shared<DesignSurface>();
        ds->register_widget_type("Button", [](){ return std::make_shared<Button>(); });
        auto w = ds->create_widget("Button");
        C(w != nullptr);
        auto n = ds->create_widget("Slider");
        C(n == nullptr);
    );

    T("designsurface_hit_test_widget",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_frame({0, 0, 400, 300});
        auto box = std::make_shared<Box>();
        box->set_frame({100, 100, 80, 60});
        ds->add_child(box);
        ds->select(box);
        InputEvent ev;
        ev.type = EventType::MouseDown;
        ev.pos = {105, 105};
        ev.button = 0;
        ds->on_event(ev);
        C(ds->selected() == box);
    );

    T("designsurface_click_empty_deselect",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_frame({0, 0, 400, 300});
        auto box = std::make_shared<Box>();
        box->set_frame({100, 100, 80, 60});
        ds->add_child(box);
        ds->select(box);
        InputEvent ev;
        ev.type = EventType::MouseDown;
        ev.pos = {5, 5};
        ev.button = 0;
        ds->on_event(ev);
        C(ds->selected() == nullptr);
    );

    T("designsurface_drag_move_widget",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_frame({0, 0, 400, 300});
        ds->set_snap_enabled(false);
        auto box = std::make_shared<Box>();
        box->set_frame({100, 100, 80, 60});
        ds->add_child(box);
        ds->select(box);
        {
            InputEvent ev;
            ev.type = EventType::MouseDown;
            ev.pos = {120, 120};
            ev.button = 0;
            ds->on_event(ev);
        }
        {
            InputEvent ev;
            ev.type = EventType::MouseMove;
            ev.pos = {150, 140};
            ds->on_event(ev);
        }
        {
            InputEvent ev;
            ev.type = EventType::MouseUp;
            ev.pos = {150, 140};
            ev.button = 0;
            ds->on_event(ev);
        }
        CF(box->frame().x, 130, 1.0f);
        CF(box->frame().y, 120, 1.0f);
    );

    T("designsurface_drag_resize_corner",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_frame({0, 0, 400, 300});
        auto box = std::make_shared<Box>();
        box->set_frame({100, 100, 80, 60});
        ds->add_child(box);
        ds->select(box);
        {
            InputEvent ev;
            ev.type = EventType::MouseDown;
            ev.pos = {180, 160};
            ev.button = 0;
            ds->on_event(ev);
        }
        {
            InputEvent ev;
            ev.type = EventType::MouseMove;
            ev.pos = {200, 180};
            ds->on_event(ev);
        }
        {
            InputEvent ev;
            ev.type = EventType::MouseUp;
            ev.pos = {200, 180};
            ev.button = 0;
            ds->on_event(ev);
        }
        C(box->frame().width > 80);
        C(box->frame().height > 60);
    );

    T("designsurface_bring_to_front",
        auto ds = std::make_shared<DesignSurface>();
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        ds->add_child(b1);
        ds->add_child(b2);
        ds->select(b1);
        ds->bring_to_front();
        C(ds->children().back() == b1);
    );

    T("designsurface_send_to_back",
        auto ds = std::make_shared<DesignSurface>();
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        ds->add_child(b1);
        ds->add_child(b2);
        ds->select(b2);
        ds->send_to_back();
        C(ds->children().front() == b2);
    );

    T("designsurface_nudge",
        auto ds = std::make_shared<DesignSurface>();
        auto box = std::make_shared<Box>();
        box->set_frame({100, 100, 80, 60});
        ds->add_child(box);
        ds->select(box);
        ds->nudge(10, -5);
        CF(box->frame().x, 110, 0.1f);
        CF(box->frame().y, 95, 0.1f);
    );

    T("designsurface_align_left",
        auto ds = std::make_shared<DesignSurface>();
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        b1->set_frame({10, 10, 50, 50});
        b2->set_frame({70, 20, 50, 50});
        ds->add_child(b1);
        ds->add_child(b2);
        ds->select(b2);
        ds->align_selected(AlignEdge::Left);
        CF(b2->frame().x, 10, 0.1f);
    );

    T("designsurface_align_top",
        auto ds = std::make_shared<DesignSurface>();
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        b1->set_frame({10, 10, 50, 50});
        b2->set_frame({70, 70, 50, 50});
        ds->add_child(b1);
        ds->add_child(b2);
        ds->select(b2);
        ds->align_selected(AlignEdge::Top);
        CF(b2->frame().y, 10, 0.1f);
    );

    T("designsurface_distribute_horizontal",
        auto ds = std::make_shared<DesignSurface>();
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        auto b3 = std::make_shared<Box>();
        b1->set_frame({10, 10, 30, 30});
        b2->set_frame({70, 10, 30, 30});
        b3->set_frame({200, 10, 30, 30});
        ds->add_child(b1);
        ds->add_child(b2);
        ds->add_child(b3);
        ds->select(b1);
        ds->add_to_selection(b2);
        ds->add_to_selection(b3);
        ds->distribute_horizontal();
        float g1 = b2->frame().x - b1->frame().right();
        float g2 = b3->frame().x - b2->frame().right();
        CF(g1, g2, 1.0f);
    );

    T("designsurface_distribute_vertical",
        auto ds = std::make_shared<DesignSurface>();
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        auto b3 = std::make_shared<Box>();
        b1->set_frame({10, 10, 30, 30});
        b2->set_frame({10, 70, 30, 30});
        b3->set_frame({10, 200, 30, 30});
        ds->add_child(b1);
        ds->add_child(b2);
        ds->add_child(b3);
        ds->select(b1);
        ds->add_to_selection(b2);
        ds->add_to_selection(b3);
        ds->distribute_vertical();
        float g1 = b2->frame().y - b1->frame().bottom();
        float g2 = b3->frame().y - b2->frame().bottom();
        CF(g1, g2, 1.0f);
    );

    T("designsurface_box_select_multiple",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_frame({0, 0, 400, 300});
        auto b1 = std::make_shared<Box>();
        auto b2 = std::make_shared<Box>();
        auto b3 = std::make_shared<Box>();
        b1->set_frame({10, 10, 50, 50});
        b2->set_frame({30, 30, 50, 50});
        b3->set_frame({200, 200, 50, 50});
        ds->add_child(b1);
        ds->add_child(b2);
        ds->add_child(b3);
        {
            InputEvent ev;
            ev.type = EventType::MouseDown;
            ev.pos = {0, 0};
            ev.button = 0;
            ds->on_event(ev);
        }
        {
            InputEvent ev;
            ev.type = EventType::MouseMove;
            ev.pos = {100, 100};
            ds->on_event(ev);
        }
        {
            InputEvent ev;
            ev.type = EventType::MouseUp;
            ev.pos = {100, 100};
            ev.button = 0;
            ds->on_event(ev);
        }
        CE(ds->selection().size(), (size_t)2);
        C(ds->is_selected(b1));
        C(ds->is_selected(b2));
        C(!ds->is_selected(b3));
    );

    T("designsurface_paint_without_selection",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_frame({0, 0, 400, 300});
        auto box = std::make_shared<Box>();
        box->set_frame({50, 50, 100, 80});
        ds->add_child(box);
        PaintContext ctx;
        ctx.set_viewport({400, 300});
        ds->on_paint(ctx);
        C(ctx.commands().size() >= 1);
    );

    T("designsurface_paint_with_selection",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_frame({0, 0, 400, 300});
        auto box = std::make_shared<Box>();
        box->set_frame({50, 50, 100, 80});
        ds->add_child(box);
        ds->select(box);
        PaintContext ctx;
        ctx.set_viewport({400, 300});
        ds->on_paint(ctx);
        C(ctx.commands().size() >= 2);
    );

    T("designsurface_key_nudge_arrows",
        auto ds = std::make_shared<DesignSurface>();
        auto box = std::make_shared<Box>();
        box->set_frame({100, 100, 80, 60});
        ds->add_child(box);
        ds->select(box);
        {
            InputEvent ev;
            ev.type = EventType::KeyDown;
            ev.key = 263;
            ds->on_event(ev);
        }
        CF(box->frame().x, 92, 0.1f);
        {
            InputEvent ev;
            ev.type = EventType::KeyDown;
            ev.key = 264;
            ds->on_event(ev);
        }
        CF(box->frame().y, 108, 0.1f);
    );

    T("designsurface_key_delete_widget",
        auto ds = std::make_shared<DesignSurface>();
        auto box = std::make_shared<Box>();
        ds->add_child(box);
        ds->select(box);
        CE((int)ds->children().size(), 1);
        {
            InputEvent ev;
            ev.type = EventType::KeyDown;
            ev.key = 127;
            ds->on_event(ev);
        }
        CE((int)ds->children().size(), 0);
        C(ds->selected() == nullptr);
    );

    T("designsurface_key_escape_deselect",
        auto ds = std::make_shared<DesignSurface>();
        auto box = std::make_shared<Box>();
        ds->add_child(box);
        ds->select(box);
        {
            InputEvent ev;
            ev.type = EventType::KeyDown;
            ev.key = 27;
            ds->on_event(ev);
        }
        C(ds->selected() == nullptr);
    );

    T("designsurface_selection_callback",
        bool called = false;
        auto ds = std::make_shared<DesignSurface>();
        ds->on_selection_changed = [&](WidgetPtr w) { called = true; };
        auto box = std::make_shared<Box>();
        ds->add_child(box);
        ds->select(box);
        C(called);
    );

    T("designsurface_move_callback",
        bool moved = false;
        auto ds = std::make_shared<DesignSurface>();
        ds->on_widget_moved = [&](WidgetPtr w, Rect r) { moved = true; };
        auto box = std::make_shared<Box>();
        box->set_frame({100, 100, 80, 60});
        ds->add_child(box);
        ds->select(box);
        {
            InputEvent ev;
            ev.type = EventType::MouseDown;
            ev.pos = {120, 120};
            ev.button = 0;
            ds->on_event(ev);
        }
        {
            InputEvent ev;
            ev.type = EventType::MouseMove;
            ev.pos = {200, 200};
            ds->on_event(ev);
        }
        {
            InputEvent ev;
            ev.type = EventType::MouseUp;
            ev.pos = {200, 200};
            ev.button = 0;
            ds->on_event(ev);
        }
        C(moved);
    );

    T("designsurface_pan_offset",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_pan({50, -30});
        CF(ds->pan().x, 50, 0.1f);
        CF(ds->pan().y, -30, 0.1f);
    );

    T("designsurface_toggle_flags",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_show_grid(false);
        ds->set_show_handles(false);
        ds->set_show_guides(false);
        ds->set_grid_enabled(false);
        ds->set_snap_enabled(false);
        C(true);
    );

    T("designsurface_min_widget_size",
        auto ds = std::make_shared<DesignSurface>();
        ds->set_min_widget_size({20, 20});
        C(true);
    );

    printf("\n  Phase 36 Results: %d passed, %d failed\n\n", p, f);
    return f == 0 ? 0 : 1;
}
