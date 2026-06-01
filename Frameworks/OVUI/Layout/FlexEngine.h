#pragma once

#include <Core/Types.h>
#include <vector>
#include <functional>
#include <memory>

namespace ovui {

enum class FlexDirection { Row, Column, RowReverse, ColumnReverse };
enum class JustifyContent { FlexStart, FlexEnd, Center, SpaceBetween, SpaceAround, SpaceEvenly };
enum class AlignItems { FlexStart, FlexEnd, Center, Stretch, Baseline };
enum class AlignContent { FlexStart, FlexEnd, Center, Stretch, SpaceBetween, SpaceAround };
enum class FlexWrap { NoWrap, Wrap, WrapReverse };

struct LayoutConstraints {
    float min_width = 0, max_width = INFINITY;
    float min_height = 0, max_height = INFINITY;
    float flex_grow = 0, flex_shrink = 1, flex_basis = -1;
    float aspect_ratio = 0;
};

struct LayoutResult {
    Rect frame;
    bool dirty = true;
};

struct LayoutNode {
    WidgetID id = 0;
    LayoutConstraints constraints;
    LayoutResult result;
    std::vector<LayoutNode*> children;
    LayoutNode* parent = nullptr;

    FlexDirection direction = FlexDirection::Column;
    JustifyContent justify = JustifyContent::FlexStart;
    AlignItems align = AlignItems::Stretch;
    AlignContent align_content = AlignContent::Stretch;
    FlexWrap wrap = FlexWrap::NoWrap;
    EdgeInsets padding;
    float gap = 0;
    float cross_gap = 0;

    Size measure_content(float available_width, float available_height) const;

    bool is_dirty() const { return result.dirty; }
    void mark_dirty();
};

class FlexEngine {
public:
    static void arrange(LayoutNode* node);

    static Size compute_intrinsic(LayoutNode* node);
    static void layout_pass(LayoutNode* root, Size viewport);

private:
    float compute_flex_base(LayoutNode* node, float container_main, bool is_row) const;
    void resolve_flex_lines(LayoutNode* container, bool is_row,
                            std::vector<std::vector<LayoutNode*>>& lines) const;
    void distribute_main_space(LayoutNode* container,
                               const std::vector<std::vector<LayoutNode*>>& lines,
                               float container_main, bool is_row);
    void align_cross_axis(LayoutNode* container,
                          const std::vector<std::vector<LayoutNode*>>& lines,
                          float container_cross, bool is_row);
};

} // namespace ovui
