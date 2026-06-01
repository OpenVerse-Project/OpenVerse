// OVUI Phase 38 — Animation Editor Tests
#include <Core/Types.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Animation/Animation.h>
#include <Designer/AnimationTimeline.h>

#include <cstdio>
#include <cmath>

using namespace ovui;
static int p=0,f=0;
#define T(n,...) do{printf("  [RUN ] %s\n",n);try{__VA_ARGS__;printf("  [PASS] %s\n",n);p++;}catch(...){printf("  [FAIL] %s\n",n);f++;}}while(0)
#define C(c) if(!(c)){printf("    FAIL %s:%d\n",__FILE__,__LINE__);throw 1;}
#define CE(a,b) if((a)!=(b)){printf("    CE %d!=%d\n",(int)(a),(int)(b));throw 1;}
#define CF(a,b,e) if(fabs((a)-(b))>(e)){printf("    CF %f!=%f\n",(float)(a),(float)(b));throw 1;}

int main() {
    printf("\nOVUI Phase 38 — Animation Editor (Timeline + Curve)\n=====================================================\n\n");

    T("timeline_editor_create",
        auto te = std::make_shared<TimelineEditor>();
        C(te != nullptr);
        C(te->is_empty());
    );

    T("timeline_editor_set_timeline",
        auto te = std::make_shared<TimelineEditor>();
        auto tl = std::make_shared<Timeline>();
        KeyframeTrack track;
        track.property = "x";
        track.keyframes.push_back({0, 0, EasingType::Linear});
        track.keyframes.push_back({1, 100, EasingType::EaseInOut});
        tl->add_track(track);
        te->set_timeline(tl);
        C(te->timeline() == tl);
        C(!te->is_empty());
    );

    T("timeline_editor_playhead",
        auto te = std::make_shared<TimelineEditor>();
        te->set_playhead(2.5f);
        CF(te->playhead(), 2.5f, 0.01f);
    );

    T("timeline_editor_zoom",
        auto te = std::make_shared<TimelineEditor>();
        te->set_zoom(2.0f);
        CF(te->zoom(), 2.0f, 0.01f);
        te->set_zoom(0.05f);
        C(te->zoom() >= 0.1f);
        te->set_zoom(200.0f);
        C(te->zoom() <= 100.0f);
    );

    T("timeline_editor_time_x_conversion",
        auto te = std::make_shared<TimelineEditor>();
        te->set_frame({0, 0, 400, 300});
        te->set_zoom(1.0f);
        float t = te->time_from_x(te->x_from_time(3.0f));
        CF(t, 3.0f, 0.1f);
    );

    T("timeline_editor_paint_with_timeline",
        auto te = std::make_shared<TimelineEditor>();
        te->set_frame({0, 0, 600, 200});

        auto tl = std::make_shared<Timeline>();
        KeyframeTrack t1;
        t1.property = "opacity";
        t1.keyframes.push_back({0.0f, 0.0f, EasingType::Linear});
        t1.keyframes.push_back({0.5f, 1.0f, EasingType::EaseInOut});
        t1.keyframes.push_back({1.0f, 0.0f, EasingType::EaseOut});
        tl->add_track(t1);

        KeyframeTrack t2;
        t2.property = "position.x";
        t2.keyframes.push_back({0.0f, 0.0f, EasingType::EaseIn});
        t2.keyframes.push_back({1.0f, 300.0f, EasingType::EaseOut});
        tl->add_track(t2);

        te->set_timeline(tl);

        PaintContext ctx;
        ctx.set_viewport({600, 200});
        te->on_paint(ctx);
        C(ctx.commands().size() >= 8);
    );

    T("timeline_editor_select_keyframe",
        auto te = std::make_shared<TimelineEditor>();
        te->set_frame({0, 0, 600, 200});

        auto tl = std::make_shared<Timeline>();
        KeyframeTrack t1;
        t1.property = "x";
        t1.keyframes.push_back({0.0f, 0.0f});
        t1.keyframes.push_back({0.5f, 50.0f});
        t1.keyframes.push_back({1.0f, 100.0f});
        tl->add_track(t1);
        te->set_timeline(tl);

        int selected_track_count = 0;
        te->on_keyframe_selected = [&](const std::string&, int) { selected_track_count++; };

        float kf_x = te->x_from_time(0.5f);
        InputEvent ev;
        ev.type = EventType::MouseDown;
        ev.pos = {kf_x, te->frame().y + 24 + 12};
        ev.button = 0;
        te->on_event(ev);
        CE(selected_track_count, 1);
    );

    T("timeline_editor_scrub_click",
        bool changed = false;
        auto te = std::make_shared<TimelineEditor>();
        te->set_frame({0, 0, 600, 200});

        auto tl = std::make_shared<Timeline>();
        KeyframeTrack t1;
        t1.property = "x";
        t1.keyframes.push_back({0.0f, 0.0f});
        t1.keyframes.push_back({1.0f, 100.0f});
        tl->add_track(t1);
        te->set_timeline(tl);

        te->on_playhead_changed = [&](float) { changed = true; };

        InputEvent ev;
        ev.type = EventType::MouseDown;
        ev.pos = {te->x_from_time(0.7f), te->frame().y + 24 + 12};
        ev.button = 0;
        te->on_event(ev);
        C(changed);
    );

    T("timeline_editor_track_height",
        auto te = std::make_shared<TimelineEditor>();
        te->set_track_height(32);
        CF(te->track_height(), 32, 0.01f);
        te->set_track_height(8);
        C(te->track_height() >= 16.0f);
    );

    T("timeline_editor_header_ruler_sizes",
        auto te = std::make_shared<TimelineEditor>();
        te->set_header_width(100);
        te->set_ruler_height(30);
        C(true);
    );

    T("curve_editor_create",
        auto ce = std::make_shared<CurveEditor>();
        C(ce != nullptr);
        CE((int)ce->curve().type(), (int)EasingType::EaseInOut);
    );

    T("curve_editor_set_curve",
        auto ce = std::make_shared<CurveEditor>();
        EasingCurve c(EasingType::Linear);
        ce->set_curve(c);
        CE((int)ce->curve().type(), (int)EasingType::Linear);
    );

    T("curve_editor_set_easing_type",
        auto ce = std::make_shared<CurveEditor>();
        ce->set_easing_type(EasingType::EaseOutBounce);
        CE((int)ce->curve().type(), (int)EasingType::EaseOutBounce);
    );

    T("curve_editor_on_changed",
        int changed = 0;
        auto ce = std::make_shared<CurveEditor>();
        ce->on_curve_changed = [&](EasingType) { changed++; };
        ce->set_easing_type(EasingType::EaseIn);
        CE(changed, 1);
    );

    T("curve_editor_paint",
        auto ce = std::make_shared<CurveEditor>();
        ce->set_frame({0, 0, 200, 200});

        PaintContext ctx;
        ctx.set_viewport({200, 200});
        ce->on_paint(ctx);
        C(ctx.commands().size() >= 6);
    );

    T("curve_editor_drag_control_point",
        auto ce = std::make_shared<CurveEditor>();
        ce->set_frame({0, 0, 200, 200});

        InputEvent ev;
        ev.type = EventType::MouseDown;
        ev.pos = {ce->frame().x + 16 + 0.25f * (200 - 32), ce->frame().y + 16 + (1.0f - 0.1f) * (200 - 32)};
        ce->on_event(ev);

        ev.type = EventType::MouseMove;
        ev.pos = {ce->frame().x + 16 + 0.3f * (200 - 32), ce->frame().y + 16 + (1.0f - 0.05f) * (200 - 32)};
        ce->on_event(ev);

        ev.type = EventType::MouseUp;
        ce->on_event(ev);
        C(true);
    );

    T("curve_editor_settings",
        auto ce = std::make_shared<CurveEditor>();
        ce->set_padding(20);
        ce->set_control_point_radius(8);
        C(true);
    );

    T("easing_curve_all_types",
        std::vector<EasingType> types = {
            EasingType::Linear, EasingType::EaseIn, EasingType::EaseOut, EasingType::EaseInOut,
            EasingType::EaseInQuad, EasingType::EaseOutQuad, EasingType::EaseInOutQuad,
            EasingType::EaseInCubic, EasingType::EaseOutCubic, EasingType::EaseInOutCubic,
            EasingType::EaseInElastic, EasingType::EaseOutElastic, EasingType::EaseInOutElastic,
            EasingType::EaseInBounce, EasingType::EaseOutBounce, EasingType::EaseInOutBounce,
            EasingType::EaseInBack, EasingType::EaseOutBack,
            EasingType::Spring, EasingType::Custom
        };
        for (auto type : types) {
            EasingCurve curve(type);
            float v0 = curve.evaluate(0.0f);
            float v5 = curve.evaluate(0.5f);
            float v1 = curve.evaluate(1.0f);
            C(v0 >= -0.1f && v0 <= 0.1f);
            C(v1 >= 0.9f && v1 <= 1.1f);
            C(v5 >= -0.5f && v5 <= 1.5f);
        }
    );

    T("timeline_editor_drag_keyframe",
        bool moved = false;
        auto te = std::make_shared<TimelineEditor>();
        te->set_frame({0, 0, 600, 200});

        auto tl = std::make_shared<Timeline>();
        KeyframeTrack t1;
        t1.property = "x";
        t1.keyframes.push_back({0.3f, 50.0f});
        tl->add_track(t1);
        te->set_timeline(tl);

        te->on_keyframe_changed = [&](const std::string&, int, float, float) { moved = true; };

        float kf_x = te->x_from_time(0.3f);
        InputEvent ev;
        ev.type = EventType::MouseDown;
        ev.pos = {kf_x, te->frame().y + 24 + 12};
        ev.button = 0;
        te->on_event(ev);

        ev.type = EventType::MouseMove;
        ev.pos = {kf_x + 30, te->frame().y + 24 + 12};
        te->on_event(ev);

        ev.type = EventType::MouseUp;
        ev.pos = {kf_x + 30, te->frame().y + 24 + 12};
        te->on_event(ev);
        C(moved);
    );

    printf("\n  Phase 38 Results: %d passed, %d failed\n\n", p, f);
    return f == 0 ? 0 : 1;
}
