// OVUI Framework — Advanced Widget Tests
#include <Core/Types.h>
#include <Reactive/Reactive.h>
#include <Layout/FlexEngine.h>
#include <Styling/Styling.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Runtime/Scheduler.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <cstring>

using namespace ovui;

static int passed = 0, failed = 0;
#define RUN_TEST(name, ...) do { printf("  [RUN ] %s\n", name); try { __VA_ARGS__; printf("  [PASS] %s\n", name); passed++; } catch(...) { printf("  [FAIL] %s\n", name); failed++; } } while(0)
#define CHECK(cond) if(!(cond)){printf("    CHECK FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CHECK_EQ(a,b) if((a)!=(b)){printf("    CHECK_EQ FAIL %d!=%d\n",(int)(a),(int)(b));throw 1;}

int main() {
    printf("\nOVUI Advanced Widgets — Test Suite\n====================================\n\n");

    RUN_TEST("checkbox_toggle",
        auto cb = std::make_shared<Checkbox>();
        cb->set_frame({0,0,200,24});
        CHECK(!cb->checked());
        InputEvent ev; ev.type = EventType::MouseDown; ev.button = 0; ev.pos = {10,12};
        cb->on_event(ev);
        CHECK(cb->checked());
    );

    RUN_TEST("radio_group",
        auto g = std::make_shared<RadioGroup>();
        auto r1 = std::make_shared<RadioButton>();
        auto r2 = std::make_shared<RadioButton>();
        g->add(r1); g->add(r2);
        CHECK(!r1->selected());
        g->select(r1);
        CHECK(r1->selected());
        CHECK(!r2->selected());
    );

    RUN_TEST("combobox_items",
        auto cb = std::make_shared<ComboBox>();
        cb->set_items({"Apple","Banana","Cherry"});
        cb->set_selected_index(1);
        CHECK_EQ(cb->selected_index(), 1);
    );

    RUN_TEST("spinbox_range",
        auto sb = std::make_shared<SpinBox>();
        sb->set_range(0, 50);
        sb->set_value(25);
        CHECK_EQ(sb->value(), 25);
        sb->set_value(100);
        CHECK_EQ(sb->value(), 50);
    );

    RUN_TEST("window_title",
        auto win = std::make_shared<Window>();
        win->set_title("Test");
        CHECK(win->title() == "Test");
    );

    RUN_TEST("tabview_tabs",
        auto tv = std::make_shared<TabView>();
        auto p1 = std::make_shared<Box>();
        auto p2 = std::make_shared<Box>();
        tv->add_tab("Tab1", p1);
        tv->add_tab("Tab2", p2);
        CHECK_EQ(tv->tab_count(), 2);
        CHECK_EQ(tv->active_tab(), 0);
        tv->set_active_tab(1);
        CHECK_EQ(tv->active_tab(), 1);
    );

    RUN_TEST("stackview_push_pop",
        auto sv = std::make_shared<StackView>();
        auto p1 = std::make_shared<Box>();
        auto p2 = std::make_shared<Box>();
        sv->push(p1);
        sv->push(p2);
        CHECK_EQ(sv->depth(), 2);
        CHECK(sv->top() == p2);
        sv->pop();
        CHECK_EQ(sv->depth(), 1);
        CHECK(sv->top() == p1);
    );

    RUN_TEST("splitter_ratio",
        auto sp = std::make_shared<Splitter>();
        sp->set_split_ratio(0.7f);
        CHECK(std::fabs(sp->split_ratio() - 0.7f) < 0.01f);
        sp->set_split_ratio(2.0f);
        CHECK(std::fabs(sp->split_ratio() - 0.95f) < 0.01f);
    );

    RUN_TEST("panel_collapse",
        auto pn = std::make_shared<Panel>();
        pn->set_title("Options");
        CHECK(!pn->is_collapsed());
        pn->toggle();
        CHECK(pn->is_collapsed());
    );

    RUN_TEST("listview_count",
        auto lv = std::make_shared<ListView>();
        lv->set_item_count(1000000);
        CHECK_EQ(lv->item_count(), 1000000);
        lv->set_selected_index(42);
        CHECK_EQ(lv->selected_index(), 42);
    );

    RUN_TEST("treeview_structure",
        auto tv = std::make_shared<TreeView>();
        TreeView::TreeNode root;
        root.label = "Root"; root.has_children = true;
        TreeView::TreeNode child;
        child.label = "Child"; child.depth = 1;
        root.children.push_back(child);
        tv->set_root(root);
        CHECK_EQ(tv->flat_count(), 1);
        tv->expand_all();
        CHECK_EQ(tv->flat_count(), 1);
    );

    RUN_TEST("tableview_dims",
        auto tbl = std::make_shared<TableView>();
        tbl->set_dimensions(100, 10);
        CHECK_EQ(tbl->rows(), 100);
        CHECK_EQ(tbl->cols(), 10);
        tbl->set_column_width(0, 150);
    );

    RUN_TEST("menubar_add",
        auto mb = std::make_shared<MenuBar>();
        mb->add_menu("File", {{"New", "Ctrl+N", []{}}, {"Open", "Ctrl+O", []{}}});
        mb->add_menu("Edit", {{"Cut", "Ctrl+X", []{}}});
        mb->clear();
    );

    RUN_TEST("context_menu",
        auto cm = std::make_shared<ContextMenu>();
        cm->set_items({{"Copy", "Ctrl+C", []{}}, {"", "", []{}, true, true}});
        cm->show({100,100}, nullptr);
        CHECK(cm->is_showing());
    );

    printf("\n=============================\n");
    printf("Results: %d passed, %d failed, %d total\n", passed, failed, passed + failed);
    return failed;
}
