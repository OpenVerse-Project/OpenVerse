#include "OVMLParser.h"
#include <fstream>
#include <sstream>
#include <cctype>
#include <stack>
#include <cstring>
#include <algorithm>

namespace ovui {

// ============================================================
// OVMLNode
// ============================================================
std::string OVMLNode::get_attr(const std::string& name, const std::string& def) const {
    for (auto& a : attributes) if (a.name == name) return a.value;
    return def;
}
bool OVMLNode::has_attr(const std::string& name) const {
    for (auto& a : attributes) if (a.name == name) return true;
    return false;
}
void OVMLNode::add_child(std::unique_ptr<OVMLNode> child) {
    child->parent = this;
    children.push_back(std::move(child));
}

// ============================================================
// OVMLParser Implementation
// ============================================================
struct OVMLParser::Impl {
    enum TokenType {
        TOK_EOF, TOK_LT, TOK_GT, TOK_SLASH, TOK_EQUALS,
        TOK_STRING, TOK_IDENT, TOK_LBRACE, TOK_RBRACE,
        TOK_TEXT, TOK_LT_SLASH
    };

    struct Token {
        TokenType type;
        std::string text;
        int line = 1, col = 1;
    };

    std::string source;
    size_t pos = 0;
    int line = 1, col = 1;
    std::vector<Token> tokens;
    size_t tok_pos = 0;

    void skip_whitespace_and_comments() {
        while (pos < source.size()) {
            if (source[pos] == '\n') { line++; col = 1; pos++; }
            else if (source[pos] == ' ' || source[pos] == '\t' || source[pos] == '\r') { col++; pos++; }
            else if (pos + 3 < source.size() && source.substr(pos, 4) == "<!--") {
                size_t end = source.find("-->", pos);
                if (end == std::string::npos) { pos = source.size(); return; }
                for (size_t i = pos; i < end; i++) if (source[i] == '\n') line++;
                pos = end + 3;
            }
            else break;
        }
    }

    Token read_string() {
        Token t; t.type = TOK_STRING; t.line = line; t.col = col;
        pos++; col++;
        while (pos < source.size() && source[pos] != '"') {
            if (source[pos] == '\\' && pos + 1 < source.size()) { pos += 2; col += 2; t.text += source[pos-1]; }
            else if (source[pos] == '\n') { line++; col = 0; t.text += source[pos++]; col++; }
            else { t.text += source[pos++]; col++; }
        }
        if (pos < source.size()) { pos++; col++; }
        return t;
    }

    Token read_ident() {
        Token t; t.type = TOK_IDENT; t.line = line; t.col = col;
        while (pos < source.size() && (std::isalnum(source[pos]) || source[pos] == '_' || source[pos] == '-' || source[pos] == '.' || source[pos] == ':' || source[pos] == '@')) {
            t.text += source[pos++]; col++;
        }
        return t;
    }

    Token read_text() {
        Token t; t.type = TOK_TEXT; t.line = line; t.col = col;
        while (pos < source.size() && source[pos] != '<') {
            if (source[pos] == '\n') { line++; col = 0; }
            t.text += source[pos++]; col++;
        }
        return t;
    }

    void tokenize() {
        while (pos < source.size()) {
            skip_whitespace_and_comments();
            if (pos >= source.size()) break;

            char c = source[pos];
            if (c == '<') {
                if (pos + 1 < source.size() && source[pos+1] == '/') {
                    tokens.push_back({TOK_LT_SLASH, "</", line, col});
                    pos += 2; col += 2;
                } else {
                    tokens.push_back({TOK_LT, "<", line, col});
                    pos++; col++;
                }
            } else if (c == '>') { tokens.push_back({TOK_GT, ">", line, col}); pos++; col++; }
            else if (c == '/') { tokens.push_back({TOK_SLASH, "/", line, col}); pos++; col++; }
            else if (c == '=') { tokens.push_back({TOK_EQUALS, "=", line, col}); pos++; col++; }
            else if (c == '"') { tokens.push_back(read_string()); }
            else if (c == '{') { tokens.push_back({TOK_LBRACE, "{", line, col}); pos++; col++; }
            else if (c == '}') { tokens.push_back({TOK_RBRACE, "}", line, col}); pos++; col++; }
            else if (std::isalpha(c) || c == '_') { tokens.push_back(read_ident()); }
            else { tokens.push_back(read_text()); }
        }
        tokens.push_back({TOK_EOF, "", line, col});
    }
};

OVMLParser::OVMLParser() : m_impl(std::make_unique<Impl>()) {}
OVMLParser::~OVMLParser() = default;

void OVMLParser::add_error(int line, int col, const std::string& msg) {
    m_errors.push_back({line, col, msg});
}

std::unique_ptr<OVMLNode> OVMLParser::parse(const std::string& source) {
    m_impl->source = source;
    m_impl->pos = 0; m_impl->line = 1; m_impl->col = 1;
    m_impl->tokens.clear(); m_impl->tok_pos = 0;
    m_errors.clear();

    m_impl->tokenize();

    auto root = std::make_unique<OVMLNode>();
    root->type = OVMLNodeType::Root;

    auto& tokens = m_impl->tokens;
    std::stack<OVMLNode*> node_stack;
    node_stack.push(root.get());

    while (m_impl->tok_pos < tokens.size()) {
        auto& tok = tokens[m_impl->tok_pos];

        if (tok.type == Impl::TOK_TEXT) {
            std::string trimmed = tok.text;
            size_t start = trimmed.find_first_not_of(" \t\r\n");
            if (start != std::string::npos) {
                auto text_node = std::make_unique<OVMLNode>();
                text_node->type = OVMLNodeType::Widget;
                text_node->name = "Text";
                text_node->text_content = trimmed;
                node_stack.top()->add_child(std::move(text_node));
            }
            m_impl->tok_pos++;
            continue;
        }

        if (tok.type == Impl::TOK_LT) {
            m_impl->tok_pos++;
            if (m_impl->tok_pos >= tokens.size()) break;

            auto& name_tok = tokens[m_impl->tok_pos];
            if (name_tok.type != Impl::TOK_IDENT) {
                add_error(name_tok.line, name_tok.col, "Expected widget name after '<'");
                break;
            }

            auto node = std::make_unique<OVMLNode>();
            node->name = name_tok.text;

            m_impl->tok_pos++;
            bool self_closing = false;

            while (m_impl->tok_pos < tokens.size()) {
                auto& at = tokens[m_impl->tok_pos];
                if (at.type == Impl::TOK_GT) { m_impl->tok_pos++; break; }
                if (at.type == Impl::TOK_SLASH) {
                    m_impl->tok_pos++;
                    if (m_impl->tok_pos < tokens.size() && tokens[m_impl->tok_pos].type == Impl::TOK_GT) {
                        m_impl->tok_pos++; self_closing = true; break;
                    }
                    continue;
                }
                if (at.type == Impl::TOK_IDENT) {
                    std::string attr_name = at.text;
                    m_impl->tok_pos++;
                    if (m_impl->tok_pos < tokens.size() && tokens[m_impl->tok_pos].type == Impl::TOK_EQUALS) {
                        m_impl->tok_pos++;
                        if (m_impl->tok_pos < tokens.size() && tokens[m_impl->tok_pos].type == Impl::TOK_STRING) {
                            node->attributes.push_back({attr_name, tokens[m_impl->tok_pos].text});
                            m_impl->tok_pos++;
                        }
                    }
                    continue;
                }
                m_impl->tok_pos++;
            }

            if (!self_closing) {
                node_stack.top()->add_child(std::move(node));
                node_stack.push(node_stack.top()->children.back().get());
            } else {
                node_stack.top()->add_child(std::move(node));
            }
            continue;
        }

        if (tok.type == Impl::TOK_LT_SLASH) {
            m_impl->tok_pos++;
            if (m_impl->tok_pos < tokens.size() && tokens[m_impl->tok_pos].type == Impl::TOK_IDENT) {
                m_impl->tok_pos++;
            }
            if (m_impl->tok_pos < tokens.size() && tokens[m_impl->tok_pos].type == Impl::TOK_GT) {
                m_impl->tok_pos++;
            }
            if (node_stack.size() > 1) node_stack.pop();
            continue;
        }

        m_impl->tok_pos++;
    }

    return root;
}

std::unique_ptr<OVMLNode> OVMLParser::parse_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) return nullptr;
    std::stringstream ss; ss << f.rdbuf();
    return parse(ss.str());
}

// ============================================================
// Validator
// ============================================================
static const char* KNOWN_WIDGETS[] = {
    "Window", "Dialog", "Container", "Box", "Text", "Button",
    "Checkbox", "RadioButton", "ComboBox", "SpinBox", "Slider",
    "TextInput", "ScrollArea", "Splitter", "Panel",
    "TabView", "StackView", "ListView", "TreeView", "TableView",
    "MenuBar", "ContextMenu", "VBox", "HBox", "Image", "Canvas"
};

bool OVMLValidator::validate(const OVMLNode& root, std::vector<std::string>& warnings) {
    bool ok = true;
    std::function<void(const OVMLNode*)> walk = [&](const OVMLNode* n) {
        if (n->type == OVMLNodeType::Widget && !n->name.empty()) {
            if (!validate_widget_type(n->name)) {
                std::string sug = suggest_widget_type(n->name);
                warnings.push_back("Unknown widget type '" + n->name + "'" + (sug.empty() ? "" : ". Did you mean '" + sug + "'?"));
                ok = false;
            }
        }
        for (auto& c : n->children) walk(c.get());
    };
    walk(&root);
    return ok;
}

bool OVMLValidator::validate_widget_type(const std::string& type) {
    for (auto* kw : KNOWN_WIDGETS) if (type == kw) return true;
    return false;
}

std::string OVMLValidator::suggest_widget_type(const std::string& partial) {
    std::string best; int best_score = 999;
    for (auto* kw : KNOWN_WIDGETS) {
        int score = 0;
        size_t min_len = std::min(partial.size(), std::string(kw).size());
        for (size_t i = 0; i < min_len; i++) {
            if (std::tolower(partial[i]) == std::tolower(kw[i])) score++;
        }
        score += std::abs((int)partial.size() - (int)std::string(kw).size()) * 2;
        if (score < best_score) { best_score = score; best = kw; }
    }
    return (best_score < partial.size() * 2) ? best : "";
}

// ============================================================
// OVMLWidgetFactory
// ============================================================
OVMLWidgetFactory& OVMLWidgetFactory::instance() {
    static OVMLWidgetFactory f;
    return f;
}

void OVMLWidgetFactory::register_type(const std::string& type, FactoryFn factory) {
    m_factories[type] = std::move(factory);
    m_types.push_back(type);
}

bool OVMLWidgetFactory::has_type(const std::string& type) const {
    return m_factories.count(type) > 0;
}

void* OVMLWidgetFactory::create(const std::string& type, const OVMLNode& node) const {
    auto it = m_factories.find(type);
    if (it != m_factories.end()) return it->second(node);
    return nullptr;
}

} // namespace ovui
