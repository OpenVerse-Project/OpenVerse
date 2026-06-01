#include "OVSSCompiler.h"
#include <sstream>
#include <cstring>
#include <cctype>
#include <algorithm>
#include <stack>
#include <utility>

namespace ovui {

std::vector<StyleValue> CompiledStyleTable::resolve(const std::string& type, WidgetState state) const {
    std::vector<StyleValue> result;
    for (auto& e : entries) {
        if (e.widget_type == type && (e.state_mask == WidgetState::None || !!(state & e.state_mask))) {
            for (auto& v : e.values) {
                bool replaced = false;
                for (auto& r : result) { if (r.prop == v.prop) { r = v; replaced = true; break; } }
                if (!replaced) result.push_back(v);
            }
        }
    }
    return result;
}

bool CompiledStyleTable::save(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    uint32_t magic = MAGIC; f.write((char*)&magic, 4);
    uint32_t ver = VERSION; f.write((char*)&ver, 4);
    uint32_t n = (uint32_t)entries.size(); f.write((char*)&n, 4);

    for (auto& e : entries) {
        uint32_t nlen = (uint32_t)e.widget_type.size();
        f.write((char*)&nlen, 4);
        f.write(e.widget_type.data(), nlen);
        uint32_t sm = (uint32_t)e.state_mask;
        f.write((char*)&sm, 4);
        uint32_t vc = (uint32_t)e.values.size();
        f.write((char*)&vc, 4);
        f.write((char*)e.values.data(), vc * sizeof(StyleValue));
    }
    return true;
}

std::unique_ptr<CompiledStyleTable> CompiledStyleTable::load(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return nullptr;
    uint32_t magic, ver, n;
    f.read((char*)&magic, 4); if (magic != MAGIC) return nullptr;
    f.read((char*)&ver, 4); if (ver != VERSION) return nullptr;
    f.read((char*)&n, 4);

    auto table = std::make_unique<CompiledStyleTable>();
    for (uint32_t i = 0; i < n; i++) {
        CompiledStyleTable::Entry e;
        uint32_t nlen; f.read((char*)&nlen, 4);
        e.widget_type.resize(nlen); f.read(e.widget_type.data(), nlen);
        uint32_t sm; f.read((char*)&sm, 4); e.state_mask = (WidgetState)sm;
        uint32_t vc; f.read((char*)&vc, 4);
        e.values.resize(vc); f.read((char*)e.values.data(), vc * sizeof(StyleValue));
        table->entries.push_back(std::move(e));
    }
    return table;
}

struct OVSSCompiler::Impl {};

OVSSCompiler::OVSSCompiler() : m_impl(std::make_unique<Impl>()) {}
OVSSCompiler::~OVSSCompiler() = default;

void OVSSCompiler::add_error(int line, int col, const std::string& msg) {
    m_errors.push_back({line, col, msg});
}

static WidgetState parse_state(const std::string& s) {
    if (s == "hover") return WidgetState::Hover;
    if (s == "pressed") return WidgetState::Pressed;
    if (s == "focused") return WidgetState::Focused;
    if (s == "disabled") return WidgetState::Disabled;
    if (s == "active") return WidgetState::Active;
    if (s == "selected") return WidgetState::Selected;
    if (s == "dragging") return WidgetState::Dragging;
    if (s == "error") return WidgetState::Error;
    if (s == "none" || s.empty()) return WidgetState::None;
    return WidgetState::None;
}

static StylePropertyID parse_property_id(const std::string& s) {
    if (s == "background" || s == "background-color") return StylePropertyID::BackgroundColor;
    if (s == "color" || s == "foreground") return StylePropertyID::Color;
    if (s == "font-size") return StylePropertyID::FontSize;
    if (s == "font-weight") return StylePropertyID::FontWeight;
    if (s == "border-radius") return StylePropertyID::BorderRadius;
    if (s == "border-width") return StylePropertyID::BorderWidth;
    if (s == "border-color") return StylePropertyID::BorderColor;
    if (s == "padding-top") return StylePropertyID::PaddingTop;
    if (s == "padding-right") return StylePropertyID::PaddingRight;
    if (s == "padding-bottom") return StylePropertyID::PaddingBottom;
    if (s == "padding-left") return StylePropertyID::PaddingLeft;
    if (s == "margin-top") return StylePropertyID::MarginTop;
    if (s == "margin-right") return StylePropertyID::MarginRight;
    if (s == "margin-bottom") return StylePropertyID::MarginBottom;
    if (s == "margin-left") return StylePropertyID::MarginLeft;
    if (s == "opacity") return StylePropertyID::Opacity;
    if (s == "min-width") return StylePropertyID::MinWidth;
    if (s == "max-width") return StylePropertyID::MaxWidth;
    if (s == "min-height") return StylePropertyID::MinHeight;
    if (s == "max-height") return StylePropertyID::MaxHeight;
    if (s == "width") return StylePropertyID::Width;
    if (s == "height") return StylePropertyID::Height;
    if (s == "gap") return StylePropertyID::Gap;
    if (s == "cross-gap") return StylePropertyID::CrossGap;
    if (s == "shadow-color") return StylePropertyID::ShadowColor;
    if (s == "shadow-offset-x") return StylePropertyID::ShadowOffsetX;
    if (s == "shadow-offset-y") return StylePropertyID::ShadowOffsetY;
    if (s == "shadow-blur") return StylePropertyID::ShadowBlur;
    return StylePropertyID::BackgroundColor;
}

StyleValue OVSSCompiler::parse_value(const std::string& raw, StylePropertyID prop) {
    StyleValue v; v.prop = prop;
    if (raw[0] == '#') {
        uint32_t hex = (uint32_t)std::stoul(raw.substr(1), nullptr, 16);
        if (raw.size() == 7) hex = (hex << 8) | 0xFF;
        Color c = Color::from_hex(hex);
        v.f[0] = c.r; v.f[1] = c.g; v.f[2] = c.b; v.f[3] = c.a;
    } else if (raw.back() == '%') {
        v.f[0] = std::stof(raw) / 100.0f;
    } else if (raw.find("px") != std::string::npos) {
        v.f[0] = std::stof(raw.substr(0, raw.size() - 2));
    } else {
        v.f[0] = std::stof(raw);
    }
    return v;
}

std::vector<OVSSCompiler::Token> OVSSCompiler::tokenize(const std::string& source) {
    std::vector<Token> tokens;
    size_t pos = 0; int line = 1, col = 1;

    while (pos < source.size()) {
        if (source[pos] == '\n') { line++; col = 1; pos++; continue; }
        if (source[pos] == ' ' || source[pos] == '\t' || source[pos] == '\r') { col++; pos++; continue; }
        if (source[pos] == '/' && pos + 1 < source.size() && source[pos+1] == '/') {
            while (pos < source.size() && source[pos] != '\n') pos++;
            continue;
        }
        if (source[pos] == '{') { tokens.push_back({TokenType::LBrace, "{", line, col}); col++; pos++; continue; }
        if (source[pos] == '}') { tokens.push_back({TokenType::RBrace, "}", line, col}); col++; pos++; continue; }
        if (source[pos] == ':') { tokens.push_back({TokenType::Colon, ":", line, col}); col++; pos++; continue; }
        if (source[pos] == ';') { tokens.push_back({TokenType::Semi, ";", line, col}); col++; pos++; continue; }
        if (source[pos] == ',') { tokens.push_back({TokenType::Comma, ",", line, col}); col++; pos++; continue; }
        if (source[pos] == '#') {
            std::string hex = "#";
            col++; pos++;
            while (pos < source.size() && std::isxdigit(source[pos])) { hex += source[pos]; col++; pos++; }
            tokens.push_back({TokenType::HexColor, hex, line, col});
            continue;
        }
        if (std::isdigit(source[pos]) || source[pos] == '-') {
            std::string num;
            while (pos < source.size() && (std::isdigit(source[pos]) || source[pos] == '.' || source[pos] == '-' || source[pos] == '%' || source[pos] == 'p')) {
                num += source[pos]; col++; pos++;
            }
            tokens.push_back({TokenType::Number, num, line, col});
            continue;
        }
        if (std::isalpha(source[pos]) || source[pos] == '_') {
            std::string id;
            while (pos < source.size() && (std::isalnum(source[pos]) || source[pos] == '_' || source[pos] == '-')) {
                id += source[pos]; col++; pos++;
            }
            tokens.push_back({TokenType::Ident, id, line, col});
            continue;
        }
        pos++; col++;
    }
    tokens.push_back({TokenType::EOF_, "", line, col});
    return tokens;
}

void OVSSCompiler::parse_block(ASTNode& node, std::vector<Token>& tokens, size_t& pos) {
    while (pos < tokens.size()) {
        auto& tok = tokens[pos];
        if (tok.type == TokenType::RBrace) { pos++; return; }
        if (tok.type == TokenType::EOF_) return;

        if (tok.type == TokenType::Ident) {
            std::string key = tok.text;
            pos++;
            if (pos < tokens.size() && tokens[pos].type == TokenType::Colon) {
                pos++;
                if (pos < tokens.size()) {
                    std::string val = tokens[pos].text;
                    pos++;
                    if (pos < tokens.size() && tokens[pos].type == TokenType::Comma) pos++;
                    if (pos < tokens.size() && tokens[pos].type == TokenType::Semi) pos++;

                    auto prop_id = parse_property_id(key);
                    auto sv = parse_value(val, prop_id);
                    node.properties.push_back({key, sv});
                }
            } else if (key == "state") {
                pos++;
                if (pos < tokens.size() && tokens[pos].type == TokenType::Ident) {
                    ASTNode state_node;
                    state_node.state_mask = parse_state(tokens[pos].text);
                    pos++;
                    if (pos < tokens.size() && tokens[pos].type == TokenType::LBrace) {
                        pos++;
                        parse_block(state_node, tokens, pos);
                    }
                    node.state_variants.push_back(std::move(state_node));
                }
            }
            continue;
        }
        pos++;
    }
}

OVSSCompiler::ASTNode OVSSCompiler::parse_root(std::vector<Token>& tokens, size_t& pos) {
    ASTNode root;
    root.name = "root";

    while (pos < tokens.size()) {
        auto& tok = tokens[pos];
        if (tok.type == TokenType::EOF_) break;

        if (tok.type == TokenType::Ident) {
            ASTNode widget_node;
            widget_node.name = tok.text;
            pos++;
            if (pos < tokens.size() && tokens[pos].type == TokenType::LBrace) {
                pos++;
                parse_block(widget_node, tokens, pos);
            }
            root.state_variants.push_back(std::move(widget_node));
            continue;
        }
        pos++;
    }
    return root;
}

CompiledStyleTable OVSSCompiler::build_table(const ASTNode& root) {
    CompiledStyleTable table;

    for (auto& widget : root.state_variants) {
        if (!widget.properties.empty()) {
            CompiledStyleTable::Entry e;
            e.widget_type = widget.name;
            e.state_mask = widget.state_mask;
            e.values = std::vector<StyleValue>{};
            for (auto& [k, v] : widget.properties) {
                e.values.push_back(v);
            }
            if (!e.values.empty()) table.entries.push_back(std::move(e));
        }

        for (auto& sv : widget.state_variants) {
            CompiledStyleTable::Entry e;
            e.widget_type = widget.name;
            e.state_mask = sv.state_mask;
            for (auto& [k, v] : sv.properties) e.values.push_back(v);
            if (!e.values.empty()) table.entries.push_back(std::move(e));
        }
    }

    return table;
}

std::unique_ptr<CompiledStyleTable> OVSSCompiler::compile(const std::string& source) {
    auto tokens = tokenize(source);
    size_t pos = 0;
    auto ast = parse_root(tokens, pos);
    if (has_errors()) return nullptr;
    auto table = std::make_unique<CompiledStyleTable>(build_table(ast));
    return table;
}

std::unique_ptr<CompiledStyleTable> OVSSCompiler::compile_file(const std::string& path) {
    std::ifstream f(path);
    if (!f) return nullptr;
    std::stringstream ss; ss << f.rdbuf();
    return compile(ss.str());
}

} // namespace ovui
