#pragma once

#include <Styling/Styling.h>
#include <string>
#include <vector>
#include <memory>
#include <fstream>

namespace ovui {

struct OVSSParseError {
    int line, col;
    std::string message;
};

struct CompiledStyleTable {
    struct Entry {
        std::string widget_type;
        WidgetState state_mask;
        std::vector<StyleValue> values;
    };
    std::vector<Entry> entries;

    std::vector<StyleValue> resolve(const std::string& type, WidgetState state) const;
    bool save(const std::string& path) const;
    static std::unique_ptr<CompiledStyleTable> load(const std::string& path);

private:
    static constexpr uint32_t MAGIC = 0x4F565354;
    static constexpr uint32_t VERSION = 1;
};

class OVSSCompiler {
public:
    OVSSCompiler();
    ~OVSSCompiler();

    std::unique_ptr<CompiledStyleTable> compile(const std::string& source);
    std::unique_ptr<CompiledStyleTable> compile_file(const std::string& path);

    const std::vector<OVSSParseError>& errors() const { return m_errors; }
    bool has_errors() const { return !m_errors.empty(); }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    std::vector<OVSSParseError> m_errors;

    void add_error(int line, int col, const std::string& msg);

    enum class TokenType { Ident, String, Number, HexColor, LBrace, RBrace, Semi, Colon, Comma, EOF_ };
    struct Token { TokenType type; std::string text; int line, col; };

    struct ASTNode {
        std::string name;
        std::vector<std::pair<std::string, StyleValue>> properties;
        WidgetState state_mask = WidgetState::None;
        std::vector<ASTNode> state_variants;
    };

    std::vector<Token> tokenize(const std::string& source);
    ASTNode parse_root(std::vector<Token>& tokens, size_t& pos);
    void parse_block(ASTNode& node, std::vector<Token>& tokens, size_t& pos);
    StyleValue parse_value(const std::string& raw, StylePropertyID prop);
    CompiledStyleTable build_table(const ASTNode& root);
};

} // namespace ovui
