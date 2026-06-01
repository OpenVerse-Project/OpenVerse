#pragma once

#include <Core/Types.h>
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>

namespace ovui {

enum class OVMLNodeType {
    Root, Widget, Property, Binding, Event, Style, Layout
};

struct OVMLAttribute {
    std::string name;
    std::string value;
};

struct OVMLNode {
    OVMLNodeType type = OVMLNodeType::Widget;
    std::string name;
    std::string text_content;
    std::vector<OVMLAttribute> attributes;
    std::vector<std::unique_ptr<OVMLNode>> children;
    OVMLNode* parent = nullptr;

    std::string get_attr(const std::string& name, const std::string& def = "") const;
    bool has_attr(const std::string& name) const;
    void add_child(std::unique_ptr<OVMLNode> child);
    int child_count() const { return (int)children.size(); }
};

struct OVMLParseError {
    int line = 0;
    int column = 0;
    std::string message;
};

class OVMLParser {
public:
    OVMLParser();
    ~OVMLParser();

    std::unique_ptr<OVMLNode> parse(const std::string& source);
    std::unique_ptr<OVMLNode> parse_file(const std::string& path);

    const std::vector<OVMLParseError>& errors() const { return m_errors; }
    bool has_errors() const { return !m_errors.empty(); }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::vector<OVMLParseError> m_errors;

    void add_error(int line, int col, const std::string& msg);
};

class OVMLValidator {
public:
    static bool validate(const OVMLNode& root, std::vector<std::string>& warnings);
    static bool validate_widget_type(const std::string& type);
    static std::string suggest_widget_type(const std::string& partial);
};

class OVMLWidgetFactory {
public:
    using FactoryFn = std::function<void*(const OVMLNode&)>;

    static OVMLWidgetFactory& instance();

    void register_type(const std::string& type, FactoryFn factory);
    bool has_type(const std::string& type) const;
    void* create(const std::string& type, const OVMLNode& node) const;

    const std::vector<std::string>& registered_types() const { return m_types; }

private:
    std::unordered_map<std::string, FactoryFn> m_factories;
    std::vector<std::string> m_types;
};

} // namespace ovui
