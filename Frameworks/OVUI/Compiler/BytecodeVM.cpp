#include <fstream>
#include <sstream>
#include "BytecodeVM.h"
#include "OVMLParser.h"
#include <charconv>
#include <system_error>

namespace ovui {

BytecodeVM::BytecodeVM() {
    m_style_engine = std::make_shared<StyleEngine>();
    register_native_widgets();
}

void BytecodeVM::add_error(const std::string& msg) { m_errors.push_back(msg); }

void BytecodeVM::register_native_widgets() {
    register_native_widget("Container", [](){return std::make_shared<Container>();});
    register_native_widget("VBox", [](){auto c=std::make_shared<Container>();c->set_direction(FlexDirection::Column);return c;});
    register_native_widget("HBox", [](){auto c=std::make_shared<Container>();c->set_direction(FlexDirection::Row);return c;});
    register_native_widget("Box", [](){return std::make_shared<Box>();});
    register_native_widget("Text", [](){return std::make_shared<Text>();});
    register_native_widget("Button", [](){return std::make_shared<Button>();});
    register_native_widget("Checkbox", [](){return std::make_shared<Checkbox>();});
    register_native_widget("RadioButton", [](){return std::make_shared<RadioButton>();});
    register_native_widget("Slider", [](){return std::make_shared<Slider>();});
    register_native_widget("TextInput", [](){return std::make_shared<TextInput>();});
    register_native_widget("ScrollArea", [](){return std::make_shared<ScrollArea>();});
    register_native_widget("Splitter", [](){return std::make_shared<Splitter>();});
    register_native_widget("Panel", [](){return std::make_shared<Panel>();});
    register_native_widget("TabView", [](){return std::make_shared<TabView>();});
    register_native_widget("Window", [](){return std::make_shared<Window>();});
    register_native_widget("MenuBar", [](){return std::make_shared<MenuBar>();});
    register_native_widget("ListView", [](){return std::make_shared<ListView>();});
    register_native_widget("TreeView", [](){return std::make_shared<TreeView>();});
    register_native_widget("TableView", [](){return std::make_shared<TableView>();});
}

void BytecodeVM::register_native_widget(const std::string& name, std::function<WidgetPtr()> factory) {
    m_native_factories[name] = std::move(factory);
}

void BytecodeVM::set_event_handler(const std::string& name, std::function<void(Widget*)> handler) {
    m_event_handlers[name] = std::move(handler);
}

WidgetPtr BytecodeVM::widget_by_id(int32_t id) const {
    auto it = m_widgets.find(id);
    return it != m_widgets.end() ? it->second : nullptr;
}

WidgetPtr BytecodeVM::get_widget_safe(int32_t id) {
    auto it = m_widgets.find(id);
    if (it == m_widgets.end()) {
        add_error("VM: widget " + std::to_string(id) + " not found at instruction " + std::to_string(m_ip));
        return nullptr;
    }
    return it->second;
}

std::string BytecodeVM::get_string_constant(int32_t id, const char* context) {
    auto c = m_module->get_constant_string(id);
    if (!c) {
        add_error(std::string("VM: [") + context + "] constant " + std::to_string(id) +
                  " not found or not a string at instruction " + std::to_string(m_ip));
        return "";
    }
    return (*c)->str_val;
}

bool BytecodeVM::safe_stof(std::string_view s, float& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc{} && ptr == s.data() + s.size();
}

bool BytecodeVM::safe_stod(std::string_view s, double& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc{} && ptr == s.data() + s.size();
}

bool BytecodeVM::safe_stoi(std::string_view s, int& out) {
    auto [ptr, ec] = std::from_chars(s.data(), s.data() + s.size(), out);
    return ec == std::errc{} && ptr == s.data() + s.size();
}

WidgetPtr BytecodeVM::create_widget_for_type(int32_t type_str_id) {
    std::string type = get_string_constant(type_str_id, "CreateWidget");
    if (type.empty()) return nullptr;

    auto it = m_native_factories.find(type);
    if (it == m_native_factories.end()) {
        add_error("VM: unknown widget type '" + type + "' at instruction " + std::to_string(m_ip));
        return std::make_shared<Container>();
    }
    return it->second();
}

void BytecodeVM::load_module(std::shared_ptr<BytecodeModule> mod) {
    m_module = std::move(mod);
    m_widgets.clear();
    m_stack = {};
    m_root = nullptr;
    m_ip = 0;
    m_errors.clear();
}

void BytecodeVM::execute_instruction(const BytecodeInstruction& in) {
    using BC = BytecodeConstant;

    switch (in.op) {
        case BytecodeOp::CreateWidget: {
            WidgetPtr w = create_widget_for_type(in.args[0]);
            if (!w) break;
            m_widgets[in.args[1]] = w;
            break;
        }
        case BytecodeOp::SetAttribute: {
            auto w = get_widget_safe(in.args[0]);
            if (!w) break;
            std::string key = get_string_constant(in.args[1], "SetAttribute.key");
            std::string val = get_string_constant(in.args[2], "SetAttribute.value");
            if (key.empty()) break;

            if (key == "text") {
                if (auto* t = dynamic_cast<Text*>(w.get())) t->set_text(val);
                else if (auto* b = dynamic_cast<Button*>(w.get())) b->set_text(val);
                else add_error("VM: SetAttribute 'text' on non-text widget at instr " + std::to_string(m_ip));
            }
            else if (key == "title") {
                if (auto* wi = dynamic_cast<Window*>(w.get())) wi->set_title(val);
                else if (auto* p = dynamic_cast<Panel*>(w.get())) p->set_title(val);
                else add_error("VM: SetAttribute 'title' on non-window/panel widget at instr " + std::to_string(m_ip));
            }
            else if (key == "placeholder") {
                if (auto* ti = dynamic_cast<TextInput*>(w.get())) ti->set_placeholder(val);
                else add_error("VM: SetAttribute 'placeholder' on non-TextInput at instr " + std::to_string(m_ip));
            }
            else if (key == "label") {
                if (auto* cb = dynamic_cast<Checkbox*>(w.get())) cb->set_label(val);
                else if (auto* rb = dynamic_cast<RadioButton*>(w.get())) rb->set_label(val);
                else add_error("VM: SetAttribute 'label' on non-checkbox/radio at instr " + std::to_string(m_ip));
            }
            else if (key == "width") {
                double wv = 0;
                if (safe_stod(val, wv)) w->set_frame({0,0,static_cast<float>(wv), w->frame().height});
                else add_error("VM: SetAttribute 'width' invalid value '" + val + "' at instr " + std::to_string(m_ip));
            }
            else if (key == "height") {
                double hv = 0;
                if (safe_stod(val, hv)) w->set_frame({0,0,w->frame().width, static_cast<float>(hv)});
                else add_error("VM: SetAttribute 'height' invalid value '" + val + "' at instr " + std::to_string(m_ip));
            }
            else if (key == "min") {
                float v = 0;
                if (auto* s = dynamic_cast<Slider*>(w.get())) {
                    if (safe_stof(val, v)) s->set_range(v, s->value());
                    else add_error("VM: SetAttribute 'min' invalid '" + val + "'");
                }
            }
            else if (key == "max") {
                float v = 0;
                if (auto* s = dynamic_cast<Slider*>(w.get())) {
                    if (safe_stof(val, v)) s->set_range(s->value(), v);
                    else add_error("VM: SetAttribute 'max' invalid '" + val + "'");
                }
            }
            else if (key == "value") {
                float v = 0;
                if (auto* s = dynamic_cast<Slider*>(w.get())) {
                    if (safe_stof(val, v)) s->set_value(v);
                    else add_error("VM: SetAttribute 'value' invalid '" + val + "'");
                }
            }
            else if (key == "class") w->set_class(val);
            else if (key == "direction") {
                if (auto* c = dynamic_cast<Container*>(w.get())) {
                    c->set_direction(val == "row" ? FlexDirection::Row : FlexDirection::Column);
                }
            }
            else if (key == "padding") {
                float v = 0;
                if (auto* c = dynamic_cast<Container*>(w.get())) {
                    if (safe_stof(val, v)) c->set_padding({v,v,v,v});
                    else add_error("VM: SetAttribute 'padding' invalid '" + val + "'");
                }
            }
            else if (key == "gap") {
                float v = 0;
                if (auto* c = dynamic_cast<Container*>(w.get())) {
                    if (safe_stof(val, v)) c->set_gap(v);
                    else add_error("VM: SetAttribute 'gap' invalid '" + val + "'");
                }
            }
            else if (key == "enabled") w->set_enabled(val == "true");
            else if (key == "visible") w->set_visible(val == "true");
            break;
        }
        case BytecodeOp::AddChild: {
            auto parent = get_widget_safe(in.args[0]);
            auto child  = get_widget_safe(in.args[1]);
            if (parent && child) parent->add_child(child);
            break;
        }
        case BytecodeOp::SetStyleClass: {
            auto w = get_widget_safe(in.args[0]);
            if (w) {
                std::string cls = get_string_constant(in.args[1], "SetStyleClass");
                if (!cls.empty()) w->set_class(cls);
            }
            break;
        }
        case BytecodeOp::SetText: {
            auto w = get_widget_safe(in.args[0]);
            if (!w) break;
            std::string txt = get_string_constant(in.args[1], "SetText");
            if (txt.empty()) break;
            if (auto* t = dynamic_cast<Text*>(w.get())) t->set_text(txt);
            else if (auto* b = dynamic_cast<Button*>(w.get())) b->set_text(txt);
            else add_error("VM: SetText on non-text/button widget at instr " + std::to_string(m_ip));
            break;
        }
        case BytecodeOp::SetEventHandler: {
            auto w = get_widget_safe(in.args[0]);
            if (!w) break;
            std::string handler_name = get_string_constant(in.args[2], "SetEventHandler");
            if (handler_name.empty()) break;
            auto it = m_event_handlers.find(handler_name);
            if (it != m_event_handlers.end()) {
                if (auto* b = dynamic_cast<Button*>(w.get())) {
                    auto fn = it->second;
                    b->on_click = [fn, w]() { fn(w.get()); };
                } else {
                    add_error("VM: SetEventHandler on non-button widget at instr " + std::to_string(m_ip));
                }
            } else {
                add_error("VM: unknown event handler '" + handler_name + "' at instr " + std::to_string(m_ip));
            }
            break;
        }
        case BytecodeOp::SetRoot: {
            m_root = get_widget_safe(in.args[0]);
            break;
        }
        case BytecodeOp::Layout: {
            if (m_root) m_scheduler.layout({static_cast<float>(in.args[0]), static_cast<float>(in.args[1])});
            break;
        }
        case BytecodeOp::End:
        case BytecodeOp::Nop:
        default: break;
    }
}

WidgetPtr BytecodeVM::execute() {
    if (!m_module) return nullptr;
    m_ip = 0;
    while (m_ip < static_cast<int>(m_module->instructions.size())) {
        auto& in = m_module->instructions[m_ip];
        if (in.op == BytecodeOp::End) break;
        execute_instruction(in);
        m_ip++;
    }
    return m_root;
}

WidgetPtr BytecodeVM::execute_module(const BytecodeModule& mod) {
    load_module(std::make_shared<BytecodeModule>(mod));
    return execute();
}

// ============================================================
// BytecodeCompiler: OVML → Bytecode
// ============================================================
BytecodeCompiler::BytecodeCompiler() {}

static int32_t cache_str(const std::string& s, BytecodeModule& mod,
                          std::unordered_map<std::string, int32_t>& cache) {
    auto it = cache.find(s);
    if (it != cache.end()) return it->second;
    int32_t id = mod.add_constant_string(s);
    if (id < 0) return -1;
    cache[s] = id;
    return id;
}

void BytecodeCompiler::parse_node_to_bytecode(
    OVMLNode* node, BytecodeModule& mod, int32_t parent_id, int& next_id,
    std::unordered_map<std::string, int32_t>& type_cache,
    std::unordered_map<std::string, int32_t>& str_cache,
    std::unordered_map<std::string, int32_t>& attr_cache)
{
    int32_t my_id = next_id++;
    int32_t type_str_id = cache_str(node->name, mod, type_cache);
    if (type_str_id < 0) { m_errors.push_back("Too many constants"); return; }

    mod.add_instruction(BytecodeOp::CreateWidget, type_str_id, my_id);

    for (auto& attr : node->attributes) {
        int32_t key_id = cache_str(attr.name, mod, attr_cache);
        int32_t val_id = cache_str(attr.value, mod, str_cache);
        if (key_id < 0 || val_id < 0) { m_errors.push_back("Too many constants"); return; }
        mod.add_instruction(BytecodeOp::SetAttribute, my_id, key_id, val_id);
    }

    if (!node->text_content.empty()) {
        int32_t text_id = cache_str(node->text_content, mod, str_cache);
        if (text_id >= 0) mod.add_instruction(BytecodeOp::SetText, my_id, text_id);
    }

    if (parent_id >= 0) {
        mod.add_instruction(BytecodeOp::AddChild, parent_id, my_id);
    } else {
        mod.add_instruction(BytecodeOp::SetRoot, my_id);
    }

    for (auto& child : node->children) {
        parse_node_to_bytecode(child.get(), mod, my_id, next_id, type_cache, str_cache, attr_cache);
    }
}

std::unique_ptr<BytecodeModule> BytecodeCompiler::compile_ovml(std::string_view ovml_source) {
    OVMLParser parser;
    auto root = parser.parse(std::string(ovml_source));
    if (parser.has_errors()) {
        for (auto& e : parser.errors()) m_errors.push_back(e.message);
        return nullptr;
    }

    auto mod = std::make_unique<BytecodeModule>();
    int next_id = 1;
    std::unordered_map<std::string, int32_t> type_cache, str_cache, attr_cache;

    for (auto& child : root->children) {
        parse_node_to_bytecode(child.get(), *mod, -1, next_id, type_cache, str_cache, attr_cache);
    }

    mod->add_instruction(BytecodeOp::End);
    return mod;
}

std::unique_ptr<BytecodeModule> BytecodeCompiler::compile_ovml_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) return nullptr;
    std::stringstream ss; ss << f.rdbuf();
    return compile_ovml(ss.str());
}

} // namespace ovui
