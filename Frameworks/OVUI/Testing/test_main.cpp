#include <Core/Types.h>
#include <Reactive/Reactive.h>
#include <Layout/FlexEngine.h>
#include <Styling/Styling.h>
#include <Widgets/Widgets.h>
#include <Runtime/Scheduler.h>

#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <cstring>

using namespace ovui;

static int passed = 0, failed = 0;

#define RUN_TEST(name, ...) \
    do { \
        printf("  [RUN ] %s\n", name); \
        try { __VA_ARGS__; printf("  [PASS] %s\n", name); passed++; } \
        catch (...) { printf("  [FAIL] %s\n", name); failed++; } \
    } while(0)

#define CHECK(cond) if(!(cond)){printf("    CHECK FAIL: %s:%d (%s)\n",__FILE__,__LINE__,#cond);throw 1;}
#define CHECK_EQ(a,b) if((a)!=(b)){printf("    CHECK_EQ FAIL: %s:%d %d!=%d\n",__FILE__,__LINE__,(int)(a),(int)(b));throw 1;}
#define CHECK_FLOAT(a,b,e) if(std::fabs((a)-(b))>(e)){printf("    CHECK_FLOAT FAIL: %s:%d %f!=%f\n",__FILE__,__LINE__,(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Framework — Test Suite\n=============================\n\n");

    RUN_TEST("core_rect",
        Rect r{10,20,100,50};
        CHECK_FLOAT(r.right(), 110, 0.01);
        CHECK(r.contains({50,30}));
);

    RUN_TEST("core_color",
        Color c = Color::from_hex(0x4FC3F7FF);
        CHECK_FLOAT(c.r, 0.309f, 0.01);
        Color d = c.darken(0.2f);
        CHECK(d.r < c.r);
);

    RUN_TEST("core_arena",
        ArenaAllocator arena(1024);
        void* p1 = arena.alloc(128);
        CHECK(p1 != nullptr);
        arena.reset();
        CHECK_EQ(arena.used(), 0);
);


    RUN_TEST("reactive_computed",
        Observable<int> a(10), b(20);
        Computed<int> sum;
        sum.set_compute([&]() { return a.get() + b.get(); });
        CHECK_EQ(sum.get(), 30);
);

    RUN_TEST("layout_flex_row",
        LayoutNode root, a, b;
        root.direction = FlexDirection::Row;
        root.result.frame = {0,0,300,100};
        a.constraints.flex_grow = 1;
        b.constraints.flex_grow = 1;
        root.children = {&a, &b};
        FlexEngine eng; eng.arrange(&root);
        CHECK(a.result.frame.width > 50);
);

    RUN_TEST("styling_theme",
        StyleEngine se;
        se.set_default_theme();
        auto vals = se.resolve("Button", "", WidgetState::None);
        CHECK(vals.size() > 0);
);

    RUN_TEST("styling_state",
        StyleEngine se;
        se.set_default_theme();
        auto n = se.resolve("Button", "", WidgetState::None);
        auto h = se.resolve("Button", "", WidgetState::Hover);
        Color bn = se.resolve_color(StylePropertyID::BackgroundColor, n, {0,0,0,1});
        Color bh = se.resolve_color(StylePropertyID::BackgroundColor, h, {0,0,0,1});
        CHECK(bh.r > bn.r || bh.g > bn.g || bh.b > bn.b);
);

    RUN_TEST("widget_tree",
        auto root = std::make_shared<Container>();
        auto btn = std::make_shared<Button>();
        root->add_child(btn);
        CHECK_EQ(root->children().size(), 1u);
);

    RUN_TEST("widget_button_click",
        auto btn = std::make_shared<Button>();
        btn->set_text("T");
        btn->set_frame({0,0,100,30});
        int clk = 0;
        btn->on_click = [&](){clk++;};
        InputEvent ev; ev.type = EventType::MouseDown; ev.button = 0; ev.pos = {50,15};
        btn->on_event(ev);
        ev.type = EventType::MouseUp;
        btn->on_event(ev);
        CHECK_EQ(clk, 1);
);

    RUN_TEST("widget_slider",
        auto sl = std::make_shared<Slider>();
        sl->set_range(0,200);
        sl->set_value(100);
        CHECK_FLOAT(sl->value(), 100, 0.01);
);

    RUN_TEST("widget_textinput",
        auto ti = std::make_shared<TextInput>();
        ti->set_text("Hello");
        CHECK(ti->text() == "Hello");
);

    RUN_TEST("scheduler_paint",
        auto root = std::make_shared<Container>();
        root->set_direction(FlexDirection::Column);
        auto box = std::make_shared<Box>();
        root->add_child(box);
        UIScheduler s; s.set_root(root); s.layout({800,600});
        PaintContext ctx; s.paint(ctx);
        CHECK(ctx.commands().size() >= 1);
);

    RUN_TEST("paint_commands",
        PaintContext ctx;
        ctx.draw_rect({10,20,100,50}, Color::from_hex(0xFF0000FF), 4);
        CHECK_EQ(ctx.commands().size(), 1u);
);

    printf("\n=============================\n");
    printf("Results: %d passed, %d failed, %d total\n", passed, failed, passed + failed);
    return failed;
}
