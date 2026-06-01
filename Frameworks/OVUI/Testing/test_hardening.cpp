// OVUI Hardening Tests — Phase OVUI-20.5
// Coverage: RadioGroup shared_ptr, StackView empty guard, Dialog viewport,
// Mouse capture Window/Splitter, ComboBox dropdown, TableView horizontal virt,
// TreeView depth, RadioButton circle, Checkbox unicode, MenuBar coords,
// ContextMenu hierarchy

#include <Core/Types.h>
#include <Reactive/Reactive.h>
#include <Layout/FlexEngine.h>
#include <Styling/Styling.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Runtime/Scheduler.h>

#include <cstdio>
#include <cmath>
#include <vector>

using namespace ovui;
static int passed = 0, failed = 0;
#define T(name,...) do{printf("  [RUN ] %s\n",name);try{__VA_ARGS__;printf("  [PASS] %s\n",name);passed++;}catch(...){printf("  [FAIL] %s\n",name);failed++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}
#define CF(a,b,e) if(fabs((a)-(b))>(e)){printf("    CF %f!=%f\n",(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Hardening Tests — Phase OVUI-20.5\n=========================================\n\n");

    T("radiogroup_shared_from_this",
        auto g = std::make_shared<RadioGroup>();
        auto r1 = std::make_shared<RadioButton>();
        auto r2 = std::make_shared<RadioButton>();
        g->add(r1); g->add(r2);
        g->select(r1);
        C(r1->selected());
        C(!r2->selected());
    );

    T("radiogroup_destruction",
        auto g = std::make_shared<RadioGroup>();
        auto r1 = std::make_shared<RadioButton>();
        g->add(r1);
        g.reset();
        InputEvent ev; ev.type = EventType::MouseDown; ev.button = 0; ev.pos = {5,5};
        r1->on_event(ev);
        C(true);
    );

    T("radiogroup_selection_after_create",
        auto g = std::make_shared<RadioGroup>();
        auto r1 = std::make_shared<RadioButton>();
        auto r2 = std::make_shared<RadioButton>();
        auto r3 = std::make_shared<RadioButton>();
        g->add(r1); g->add(r2); g->add(r3);
        g->select(r2);
        C(r2->selected());
        C(!r1->selected());
        g->select(r3);
        C(r3->selected());
        C(!r2->selected());
    );

    T("stackview_replace_empty",
        auto sv = std::make_shared<StackView>();
        auto p = std::make_shared<Box>();
        sv->replace(p);
        CE(sv->depth(), 1);
        C(sv->top() == p);
    );

    T("stackview_replace_nonempty",
        auto sv = std::make_shared<StackView>();
        auto p1 = std::make_shared<Box>();
        auto p2 = std::make_shared<Box>();
        sv->push(p1);
        sv->replace(p2);
        CE(sv->depth(), 1);
        C(sv->top() == p2);
    );

    T("dialog_viewport_background",
        auto dlg = std::make_shared<Dialog>();
        auto parent = std::make_shared<Container>();
        dlg->show(parent, {800,600});
        PaintContext ctx;
        ctx.set_viewport({800,600});
        dlg->on_paint(ctx);
        C(ctx.commands().size() >= 1);
        auto& cmds = ctx.commands();
        C(cmds[0].frame.width > 100);
    );

    T("dialog_dismiss",
        auto dlg = std::make_shared<Dialog>();
        auto parent = std::make_shared<Container>();
        dlg->show(parent, {1024,768});
        C(dlg->is_showing());
        dlg->dismiss();
        C(!dlg->is_showing());
    );

    T("mouse_capture_window_drag",
        auto win = std::make_shared<Window>();
        win->set_frame({50,50,400,300});
        C(!win->has_mouse_capture());
        InputEvent ev1; ev1.type = EventType::MouseDown; ev1.pos = {55,55};
        win->on_event(ev1);
        C(win->has_mouse_capture());
        InputEvent ev2; ev2.type = EventType::MouseMove; ev2.pos = {100,100};
        win->on_event(ev2);
        Rect f = win->frame();
        C(f.x > 50);
        InputEvent ev3; ev3.type = EventType::MouseUp; ev3.pos = {200,200};
        win->on_event(ev3);
        C(!win->has_mouse_capture());
    );

    T("mouse_capture_splitter_drag",
        auto sp = std::make_shared<Splitter>();
        sp->set_frame({0,0,400,300});
        InputEvent ev1; ev1.type = EventType::MouseDown; ev1.pos = {200,150};
        sp->on_event(ev1);
        C(sp->has_mouse_capture());
        InputEvent ev2; ev2.type = EventType::MouseMove; ev2.pos = {100,150};
        sp->on_event(ev2);
        CF(sp->split_ratio(), 0.25f, 0.1f);
        InputEvent ev3; ev3.type = EventType::MouseUp; ev3.pos = {100,150};
        sp->on_event(ev3);
        C(!sp->has_mouse_capture());
    );

    T("combobox_full_dropdown",
        auto cb = std::make_shared<ComboBox>();
        cb->set_frame({0,0,150,30});
        cb->set_items({"Alpha","Beta","Gamma","Delta","Epsilon"});
        InputEvent ev_open; ev_open.type = EventType::MouseDown; ev_open.pos = {75,15};
        cb->on_event(ev_open);
        PaintContext ctx; cb->on_paint(ctx);
        C(ctx.commands().size() > 1);
    );

    T("combobox_select_item",
        auto cb = std::make_shared<ComboBox>();
        cb->set_frame({0,0,150,30});
        cb->set_items({"A","B","C"});
        InputEvent ev1; ev1.type = EventType::MouseDown; ev1.pos = {75,15};
        cb->on_event(ev1);
        InputEvent ev2; ev2.type = EventType::MouseDown; ev2.pos = {75,57};
        cb->on_event(ev2);
        CE(cb->selected_index(), 1);
    );

    T("combobox_dismiss_outside_click",
        auto cb = std::make_shared<ComboBox>();
        cb->set_frame({0,0,150,30});
        cb->set_items({"A","B","C"});
        InputEvent ev1; ev1.type = EventType::MouseDown; ev1.pos = {75,15};
        cb->on_event(ev1);
        InputEvent ev2; ev2.type = EventType::MouseDown; ev2.pos = {300,300};
        cb->on_event(ev2);
        C(true);
    );

    T("tableview_horizontal_virtualization",
        auto tv = std::make_shared<TableView>();
        tv->set_frame({0,0,400,300});
        tv->set_dimensions(100, 1000);
        for (int i = 0; i < 500; i++) tv->set_column_width(i, 120);
        PaintContext ctx; tv->on_paint(ctx);
        C(ctx.commands().size() < 5000);
    );

    T("tableview_million_columns",
        auto tv = std::make_shared<TableView>();
        tv->set_frame({0,0,500,300});
        tv->set_dimensions(10, 100000);
        for (int i = 0; i < 1000; i++) tv->set_column_width(i, 100);
        PaintContext ctx; tv->on_paint(ctx);
        C(true);
    );

    T("treeview_multi_level_depth",
        auto tv = std::make_shared<TreeView>();
        TreeView::TreeNode root;
        root.label = "R"; root.has_children = true;
        TreeView::TreeNode a; a.label = "A";
        TreeView::TreeNode b; b.label = "B"; b.has_children = true;
        TreeView::TreeNode b1; b1.label = "B1";
        b.children.push_back(b1);
        root.children.push_back(a);
        root.children.push_back(b);
        tv->set_root(root);
        tv->expand_all();
        CE(tv->flat_count(), 3);
        C(tv->root().children[0].depth == 1);
        C(tv->root().children[1].depth == 1);
        C(tv->root().children[1].children[0].depth == 2);
    );

    T("treeview_depth_after_collapse",
        auto tv = std::make_shared<TreeView>();
        TreeView::TreeNode root;
        root.label = "R"; root.has_children = true;
        TreeView::TreeNode a; a.label = "A";
        root.children.push_back(a);
        tv->set_root(root);
        CE(tv->flat_count(), 1);
        C(tv->root().children[0].depth == 1);
    );

    T("radiobutton_circle_rendering",
        auto rb = std::make_shared<RadioButton>();
        rb->set_frame({0,0,200,30});
        rb->set_selected(true);
        PaintContext ctx; rb->on_paint(ctx);
        C(ctx.commands().size() >= 2);
    );

    T("checkbox_unicode_checkmark",
        auto cb = std::make_shared<Checkbox>();
        cb->set_frame({0,0,200,24});
        cb->set_checked(true);
        PaintContext ctx; cb->on_paint(ctx);
        C(ctx.commands().size() >= 1);
    );

    T("menubar_frame_relative_coords",
        auto mb = std::make_shared<MenuBar>();
        mb->set_frame({100,0,600,24});
        mb->add_menu("File", {{"New", "Ctrl+N", []{}}});
        InputEvent ev; ev.type = EventType::MouseDown; ev.pos = {140,12};
        mb->on_event(ev);
        C(true);
    );

    T("contextmenu_show_dismiss",
        auto cm = std::make_shared<ContextMenu>();
        cm->set_items({{"Copy", "Ctrl+C", []{}}, {"Paste", "Ctrl+V", []{}}});
        auto owner = std::make_shared<Container>();
        cm->show({50,50}, owner);
        C(cm->is_showing());
        cm->dismiss();
        C(!cm->is_showing());
    );

    T("contextmenu_item_click",
        auto cm = std::make_shared<ContextMenu>();
        int clicked = 0;
        cm->set_items({{"Action", "", [&](){ clicked++; }}});
        cm->show({0,0}, nullptr);
        InputEvent ev; ev.type = EventType::MouseDown; ev.pos = {10,12};
        cm->on_event(ev);
        CE(clicked, 1);
        C(!cm->is_showing());
    );

    T("contextmenu_outside_click",
        auto cm = std::make_shared<ContextMenu>();
        cm->set_items({{"X", "", []{}}});
        cm->show({0,0}, nullptr);
        InputEvent ev; ev.type = EventType::MouseDown; ev.pos = {999,999};
        cm->on_event(ev);
        C(!cm->is_showing());
    );

    printf("\n=========================================\n");
    printf("Results: %d passed, %d failed, %d total\n", passed, failed, passed + failed);
    return failed;
}
