#pragma once

#include <Widgets/Widgets.h>
#include <Reactive/Reactive.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ovui {

class RadioGroup : public std::enable_shared_from_this<RadioGroup> {
public:
    void add(std::shared_ptr<class RadioButton> btn);
    void select(std::shared_ptr<class RadioButton> btn);
    std::shared_ptr<class RadioButton> selected() const { return m_selected.lock(); }
private:
    std::vector<std::weak_ptr<class RadioButton>> m_buttons;
    std::weak_ptr<class RadioButton> m_selected;
};

class Checkbox : public Widget {
public:
    Checkbox();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_label(const std::string& t) { m_label = t; }
    void set_checked(bool c) { m_checked.set(c); }
    bool checked() const { return m_checked.get(); }
    Observable<bool>& checked_obs() { return m_checked; }
    std::function<void(bool)> on_toggle;
private:
    std::string m_label;
    Observable<bool> m_checked{false};
};

class RadioButton : public Widget {
public:
    RadioButton();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_label(const std::string& t) { m_label = t; }
    void set_selected(bool s) { m_selected.set(s); }
    bool selected() const { return m_selected.get(); }
    void set_group(std::shared_ptr<RadioGroup> g);
    std::function<void()> on_select;
private:
    std::string m_label;
    Observable<bool> m_selected{false};
    std::weak_ptr<RadioGroup> m_group;
};

class ComboBox : public Widget {
public:
    ComboBox();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_items(const std::vector<std::string>& items);
    void set_selected_index(int idx);
    int selected_index() const { return m_selected_idx.get(); }
    Observable<int>& selected_idx_obs() { return m_selected_idx; }
    std::function<void(int)> on_change;
private:
    std::vector<std::string> m_items;
    Observable<int> m_selected_idx{-1};
    bool m_open = false;
    float m_popup_height = 0;
    int m_hovered_popup_idx = -1;
};

class SpinBox : public Widget {
public:
    SpinBox();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_range(int min, int max) { m_min = min; m_max = max; }
    void set_value(int v) { m_value.set(std::clamp(v, m_min, m_max)); }
    int value() const { return m_value.get(); }
    Observable<int>& value_obs() { return m_value; }
    std::function<void(int)> on_change;
private:
    int m_min = 0, m_max = 100;
    Observable<int> m_value{0};
};

class Window : public Container {
public:
    Window();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void before_event(InputEvent& ev);
    void set_title(const std::string& t) { m_title = t; }
    const std::string& title() const { return m_title; }
    void set_closable(bool c) { m_closable = c; }
    bool is_closable() const { return m_closable; }
    void set_resizable(bool r) { m_resizable = r; }
    bool is_resizable() const { return m_resizable; }
    void set_titlebar_height(float h) { m_titlebar_h = h; }
    float titlebar_height() const { return m_titlebar_h; }
    std::function<void()> on_close;
protected:
    std::string m_title = "Window";
    bool m_closable = true, m_resizable = true;
    bool m_dragging = false;
    Point m_drag_start;
    float m_titlebar_h = 30;
};

class Dialog : public Window {
public:
    Dialog();
    void on_paint(PaintContext& ctx) override;
    void set_modal(bool m) { m_modal = m; }
    bool is_modal() const { return m_modal; }
    void show(WidgetPtr parent, Size viewport);
    void dismiss();
    std::function<void()> on_dismiss;
    bool is_showing() const { return m_showing; }
protected:
    bool m_modal = true;
    bool m_showing = false;
};

class TabView : public Container {
public:
    TabView();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    int add_tab(const std::string& label, WidgetPtr content);
    void remove_tab(int index);
    void set_active_tab(int index);
    int active_tab() const { return m_active_idx; }
    int tab_count() const { return (int)m_tabs.size(); }
private:
    struct Tab { std::string label; WidgetPtr content; };
    std::vector<Tab> m_tabs;
    int m_active_idx = 0;
    float m_tab_height = 28, m_tab_padding = 8;
};

class StackView : public Container {
public:
    StackView();
    void push(WidgetPtr page);
    void pop();
    void replace(WidgetPtr page);
    WidgetPtr top() const { return m_stack.empty() ? nullptr : m_stack.back(); }
    int depth() const { return (int)m_stack.size(); }
private:
    std::vector<WidgetPtr> m_stack;
};

class Splitter : public Container {
public:
    Splitter();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_split_ratio(float r) { m_ratio = std::clamp(r, 0.05f, 0.95f); }
    float split_ratio() const { return m_ratio; }
    void set_vertical(bool v) { m_vertical = v; }
    void set_splitter_width(float w) { m_splitter_w = w; }
private:
    float m_ratio = 0.5f;
    float m_splitter_w = 4;
    bool m_vertical = false;
    bool m_dragging = false;
};

class Panel : public Container {
public:
    Panel();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_title(const std::string& t) { m_title = t; }
    void set_collapsed(bool c);
    void toggle() { set_collapsed(!m_collapsed); }
    bool is_collapsed() const { return m_collapsed; }
    void set_header_height(float h) { m_header_h = h; }
private:
    std::string m_title;
    bool m_collapsed = false;
    float m_header_h = 24, m_expanded_height = 200;
};

struct MenuItem {
    std::string label;
    std::string shortcut;
    std::function<void()> action;
    bool enabled = true;
    bool separator = false;
    std::vector<MenuItem> children;
};

class MenuBar : public Widget {
public:
    MenuBar();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void add_menu(const std::string& label, std::vector<MenuItem> items);
    void clear();
    void set_item_height(float h) { m_item_h = h; }
private:
    struct Menu { std::string label; std::vector<MenuItem> items; };
    std::vector<Menu> m_menus;
    int m_open_menu = -1;
    float m_item_h = 24, m_item_pad = 12;
};

class ContextMenu : public Widget {
public:
    ContextMenu();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_items(std::vector<MenuItem> items);
    void show(Point pos, WidgetPtr parent);
    void dismiss();
    bool is_showing() const { return m_showing; }
private:
    std::vector<MenuItem> m_items;
    bool m_showing = false;
    Point m_pos;
    float m_item_h = 22, m_min_width = 160;
    WidgetPtr m_owner;
};

class ListView : public Container {
public:
    using ItemBuilder = std::function<WidgetPtr(int index)>;
    ListView();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_item_count(int n) { m_item_count = n; m_dirty = true; }
    int item_count() const { return m_item_count; }
    void set_item_height(float h) { m_item_height = h; m_dirty = true; }
    void set_item_builder(ItemBuilder b) { m_builder = std::move(b); m_dirty = true; }
    void set_selected_index(int idx);
    int selected_index() const { return m_selected_idx; }
    void scroll_to(int index);
    void refresh();
private:
    int m_item_count = 0;
    float m_item_height = 28;
    float m_scroll_offset = 0;
    int m_selected_idx = -1;
    int m_first_visible = 0, m_last_visible = 0;
    bool m_dirty = true;
    ItemBuilder m_builder;
    void update_visible_items();
};

class TreeView : public Container {
public:
    struct TreeNode {
        std::string label;
        int depth = 0;
        bool expanded = false;
        bool has_children = false;
        bool selected = false;
        std::vector<TreeNode> children;
    };
    TreeView();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_root(TreeNode root);
    const TreeNode& root() const { return m_root; }
    void toggle_node(TreeNode* node);
    void expand_all();
    void collapse_all();
    std::function<void(TreeNode*)> on_select;
    int flat_count() const { return (int)m_flat.size(); }
private:
    TreeNode m_root;
    std::vector<TreeNode*> m_flat;
    float m_indent = 20, m_row_height = 24;
    TreeNode* m_selected = nullptr;
    void flatten();
    void assign_depths(TreeNode* node, int depth);
};

class TableView : public Container {
public:
    using CellBuilder = std::function<std::string(int row, int col)>;
    TableView();
    void on_paint(PaintContext& ctx) override;
    bool on_event(InputEvent& ev) override;
    void set_dimensions(int rows, int cols);
    int rows() const { return m_rows; }
    int cols() const { return m_cols; }
    void set_column_width(int col, float w);
    void set_row_height(float h) { m_row_height = h; m_dirty = true; }
    void set_cell_builder(CellBuilder b) { m_builder = std::move(b); m_dirty = true; }
    void set_header_labels(const std::vector<std::string>& labels);
    std::function<void(int row, int col)> on_cell_click;
    std::function<void(int col)> on_header_click;
    void refresh();
private:
    int m_rows = 0, m_cols = 0;
    float m_row_height = 24, m_header_height = 28;
    float m_scroll_x = 0, m_scroll_y = 0;
    std::vector<float> m_col_widths;
    std::vector<std::string> m_headers;
    int m_selected_row = -1, m_selected_col = -1;
    int m_first_row = 0, m_last_row = 0;
    int m_first_col = 0, m_last_col = 0;
    CellBuilder m_builder;
    bool m_dirty = true;
    void update_visible_cells();
    float total_width() const;
};

} // namespace ovui
