#pragma once

#include <Compiler/Bytecode.h>
#include <Widgets/Widgets.h>
#include <Widgets/Advanced.h>
#include <Styling/Styling.h>
#include <Runtime/Scheduler.h>
#include <vector>
#include <stack>
#include <unordered_map>
#include <memory>
#include <functional>
#include <charconv>

namespace ovui {

class BytecodeVM {
public:
    BytecodeVM();

    void load_module(std::shared_ptr<BytecodeModule> mod);
    WidgetPtr execute();
    WidgetPtr execute_module(const BytecodeModule& mod);

    void set_style_engine(std::shared_ptr<StyleEngine> se) { m_style_engine = se; }
    UIScheduler& scheduler() { return m_scheduler; }

    void register_native_widget(const std::string& name, std::function<WidgetPtr()> factory);
    void set_event_handler(const std::string& name, std::function<void(Widget*)> handler);

    const std::vector<std::string>& errors() const { return m_errors; }

    WidgetPtr widget_by_id(int32_t id) const;
    WidgetPtr root() const { return m_root; }

    static bool safe_stof(std::string_view s, float& out);
    static bool safe_stod(std::string_view s, double& out);
    static bool safe_stoi(std::string_view s, int& out);

private:
    std::shared_ptr<BytecodeModule> m_module;
    std::shared_ptr<StyleEngine> m_style_engine;
    UIScheduler m_scheduler;
    std::vector<std::string> m_errors;

    std::unordered_map<int32_t, WidgetPtr> m_widgets;
    std::unordered_map<std::string, std::function<WidgetPtr()>> m_native_factories;
    std::unordered_map<std::string, std::function<void(Widget*)>> m_event_handlers;
    std::stack<WidgetPtr> m_stack;
    WidgetPtr m_root;
    int m_ip = 0;

    void execute_instruction(const BytecodeInstruction& in);
    WidgetPtr create_widget_for_type(int32_t type_str_id);
    void register_native_widgets();

    void add_error(const std::string& msg);

    WidgetPtr get_widget_safe(int32_t id);
    std::string get_string_constant(int32_t id, const char* context);

};

class BytecodeCompiler {
public:
    BytecodeCompiler();

    std::unique_ptr<BytecodeModule> compile_ovml(std::string_view ovml_source);
    std::unique_ptr<BytecodeModule> compile_ovml_file(const std::string& path);

    const std::vector<std::string>& errors() const { return m_errors; }

private:
    std::vector<std::string> m_errors;

    void parse_node_to_bytecode(class OVMLNode* node, BytecodeModule& mod, int32_t parent_id, int& next_id,
                                std::unordered_map<std::string, int32_t>& type_cache,
                                std::unordered_map<std::string, int32_t>& str_cache,
                                std::unordered_map<std::string, int32_t>& attr_cache);
};

} // namespace ovui
