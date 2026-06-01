#pragma once
#include <Widgets/Widgets.h>
#include <Layout/FlexEngine.h>
#include <functional>
#include <chrono>

namespace ovui {

class UIScheduler {
public:
    void set_root(WidgetPtr root) { m_root = std::move(root); }
    WidgetPtr root() const { return m_root; }

    void layout(Size viewport) {
        if (!m_root) return;
        m_root->update_layout_node();
        FlexEngine::layout_pass(&m_root->layout_node(), viewport);
    }

    void paint(PaintContext& ctx) {
        if (!m_root) return;
        paint_recursive(m_root, ctx);
    }

    bool dispatch_event(InputEvent& ev) {
        if (!m_root) return false;
        return dispatch_recursive(m_root, ev);
    }

private:
    WidgetPtr m_root;

    void paint_recursive(WidgetPtr w, PaintContext& ctx) {
        if (!w->is_visible()) return;
        w->on_paint(ctx);
        for (auto& c : w->children()) paint_recursive(c, ctx);
    }

    bool dispatch_recursive(WidgetPtr w, InputEvent& ev) {
        for (auto it = w->children().rbegin(); it != w->children().rend(); ++it) {
            if (dispatch_recursive(*it, ev)) return true;
        }
        return w->on_event(ev);
    }
};

} // namespace ovui
