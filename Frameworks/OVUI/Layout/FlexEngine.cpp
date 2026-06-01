#include "FlexEngine.h"
#include <algorithm>
#include <cmath>

namespace ovui {

void LayoutNode::mark_dirty() {
    result.dirty = true;
    for (auto* child : children) child->mark_dirty();
}

Size LayoutNode::measure_content(float, float) const { return {0, 0}; }

Size FlexEngine::compute_intrinsic(LayoutNode* node) {
    if (!node) return {};
    if (node->children.empty()) return node->measure_content(INFINITY, INFINITY);
    bool is_row = node->direction == FlexDirection::Row || node->direction == FlexDirection::RowReverse;
    float main_size = 0, cross_size = 0;
    for (auto* child : node->children) {
        Size s = compute_intrinsic(child);
        if (is_row) { main_size += s.width + node->gap; cross_size = std::max(cross_size, s.height); }
        else { main_size += s.height + node->gap; cross_size = std::max(cross_size, s.width); }
    }
    if (!node->children.empty()) main_size -= node->gap;
    if (is_row) return {main_size + node->padding.horizontal(), cross_size + node->padding.vertical()};
    return {cross_size + node->padding.horizontal(), main_size + node->padding.vertical()};
}

static float calc_flex_base(LayoutNode* node, float cm, bool is_row) {
    int fc = 0; float used = node->gap * std::max(0,(int)node->children.size()-1);
    for (auto* ch : node->children) {
        float cm2 = is_row ? ch->constraints.flex_basis : ch->constraints.flex_basis;
        if (cm2 > 0) used += cm2;
        else if (ch->constraints.flex_grow > 0) fc++;
    }
    return (fc > 0 && cm > used) ? (cm - used) / (float)fc : 0;
}

static void resolve_lines(LayoutNode* n, bool is_row, std::vector<std::vector<LayoutNode*>>& lines) {
    lines.clear(); lines.push_back({});
    float ls = 0, cm = is_row ? n->result.frame.width - n->padding.horizontal() : n->result.frame.height - n->padding.vertical();
    for (auto* ch : n->children) {
        float csz = is_row ? ch->result.frame.width : ch->result.frame.height;
        if (ch->constraints.flex_basis > 0) csz = ch->constraints.flex_basis;
        if (n->wrap != FlexWrap::NoWrap && !lines.back().empty() && ls + n->gap + csz > cm) {
            lines.push_back({}); ls = 0;
        }
        lines.back().push_back(ch); ls += csz + n->gap;
    }
}

static void dist_main(LayoutNode* n, const std::vector<std::vector<LayoutNode*>>& lines, float cm, bool is_row) {
    for (auto& l : lines) {
        float tg = 0, used = 0;
        for (auto* ch : l) {
            float csz = is_row ? ch->result.frame.width : ch->result.frame.height;
            if (ch->constraints.flex_basis > 0) csz = ch->constraints.flex_basis;
            used += csz; tg += ch->constraints.flex_grow;
        }
        used += n->gap * std::max(0,(int)l.size()-1);
        float rem = cm - used, epg = tg > 0 ? rem / tg : 0;
        float off = 0;
        switch(n->justify) {
            case JustifyContent::FlexEnd: off = rem; break;
            case JustifyContent::Center: off = rem * 0.5f; break;
            case JustifyContent::SpaceBetween: if(l.size()>1) epg = rem/(float)(l.size()-1); break;
            case JustifyContent::SpaceAround: off = rem/(float)(l.size()*2); epg = rem/(float)l.size(); break;
            case JustifyContent::SpaceEvenly: off = rem/(float)(l.size()+1); epg = rem/(float)(l.size()+1); break;
            default: break;
        }
        float pos = off;
        for (auto* ch : l) {
            float csz = is_row ? ch->result.frame.width : ch->result.frame.height;
            if (ch->constraints.flex_basis > 0) csz = ch->constraints.flex_basis;
            if (ch->constraints.flex_grow > 0 && tg > 0) csz += ch->constraints.flex_grow * epg;
            if (is_row) { ch->result.frame.x = pos; ch->result.frame.width = csz; }
            else { ch->result.frame.y = pos; ch->result.frame.height = csz; }
            pos += csz;
            pos += (n->justify == JustifyContent::SpaceBetween && ch != l.back()) ? epg : n->gap;
        }
    }
}

static void align_cross(LayoutNode* n, const std::vector<std::vector<LayoutNode*>>& lines, float cc, bool is_row) {
    for (auto& l : lines) {
        for (auto* ch : l) {
            float ccr = is_row ? ch->result.frame.height : ch->result.frame.width;
            switch(n->align) {
                case AlignItems::FlexStart: if(is_row)ch->result.frame.y=0; else ch->result.frame.x=0; break;
                case AlignItems::FlexEnd: if(is_row)ch->result.frame.y=cc-ccr; else ch->result.frame.x=cc-ccr; break;
                case AlignItems::Center: if(is_row)ch->result.frame.y=(cc-ccr)*0.5f; else ch->result.frame.x=(cc-ccr)*0.5f; break;
                case AlignItems::Stretch: if(is_row){ch->result.frame.y=0;ch->result.frame.height=cc;} else{ch->result.frame.x=0;ch->result.frame.width=cc;} break;
                default: break;
            }
        }
    }
}

void FlexEngine::arrange(LayoutNode* node) {
    if (!node || node->children.empty()) return;
    bool is_row = node->direction == FlexDirection::Row || node->direction == FlexDirection::RowReverse;
    float cm = is_row ? node->result.frame.width - node->padding.horizontal() : node->result.frame.height - node->padding.vertical();
    float cc = is_row ? node->result.frame.height - node->padding.vertical() : node->result.frame.width - node->padding.horizontal();

    std::vector<std::vector<LayoutNode*>> lines;
    resolve_lines(node, is_row, lines);
    dist_main(node, lines, cm, is_row);
    align_cross(node, lines, cc, is_row);

    for (auto* ch : node->children) {
        ch->result.frame.x += node->padding.left + node->result.frame.x;
        ch->result.frame.y += node->padding.top + node->result.frame.y;
        ch->result.dirty = false;
        arrange(ch);
    }
    node->result.dirty = false;
}

void FlexEngine::layout_pass(LayoutNode* root, Size viewport) {
    if (!root) return;
    root->result.frame = {0, 0, viewport.width, viewport.height};
    FlexEngine e; e.arrange(root);
}

} // namespace ovui
