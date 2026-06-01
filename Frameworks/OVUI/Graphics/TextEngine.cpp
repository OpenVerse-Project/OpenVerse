#include "TextEngine.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include <hb.h>
#include <hb-ft.h>

#include <cstring>
#include <cmath>
#include <vector>
#include <algorithm>
#include <unordered_map>

namespace ovui {

struct TextEngine::Impl {
    FT_Library ft_library = nullptr;
    FT_Face ft_face = nullptr;
    hb_font_t* hb_font = nullptr;
    bool loaded = false;

    std::unordered_map<uint32_t, GlyphInfo> glyph_cache;
    std::vector<uint8_t> atlas_buffer;
    int atlas_width = 1024;
    int atlas_height = 1024;
    int atlas_cursor_x = 1;
    int atlas_cursor_y = 1;
    int atlas_row_height = 0;

    ~Impl() {
        if (hb_font) hb_font_destroy(hb_font);
        if (ft_face) FT_Done_Face(ft_face);
        if (ft_library) FT_Done_FreeType(ft_library);
    }

    void cache_all_ascii() {
        for (uint32_t cp = 32; cp < 127; cp++) rasterize_glyph(cp);
    }

    void rasterize_glyph(uint32_t codepoint) {
        if (!ft_face) return;
        if (glyph_cache.count(codepoint)) return;

        FT_UInt gindex = FT_Get_Char_Index(ft_face, codepoint);
        if (!gindex) return;

        FT_Load_Glyph(ft_face, gindex, FT_LOAD_RENDER | FT_LOAD_TARGET_LIGHT);
        FT_GlyphSlot slot = ft_face->glyph;

        int gw = (int)slot->bitmap.width;
        int gh = (int)slot->bitmap.rows;

        if (gw == 0 || gh == 0) {
            GlyphInfo info{};
            info.codepoint = codepoint;
            info.advance_x = (float)slot->advance.x / 64.0f;
            info.advance_y = (float)slot->advance.y / 64.0f;
            info.width = 0; info.height = 0;
            glyph_cache[codepoint] = info;
            return;
        }

        if (atlas_cursor_x + gw + 1 >= atlas_width) {
            atlas_cursor_x = 1;
            atlas_cursor_y += atlas_row_height + 1;
            atlas_row_height = 0;
        }
        if (atlas_cursor_y + gh + 1 >= atlas_height) return;

        if (atlas_buffer.empty()) atlas_buffer.resize(atlas_width * atlas_height, 0);

        for (int y = 0; y < gh; y++) {
            int src_row = y * gw;
            int dst_row = (atlas_cursor_y + y) * atlas_width + atlas_cursor_x;
            for (int x = 0; x < gw; x++) {
                atlas_buffer[dst_row + x] = slot->bitmap.buffer[src_row + x];
            }
        }

        GlyphInfo info{};
        info.codepoint = codepoint;
        info.texture_id = 0;
        info.atlas_u = (float)atlas_cursor_x / (float)atlas_width;
        info.atlas_v = (float)atlas_cursor_y / (float)atlas_height;
        info.atlas_w = (float)gw / (float)atlas_width;
        info.atlas_h = (float)gh / (float)atlas_height;
        info.bearing_x = (float)slot->bitmap_left;
        info.bearing_y = (float)slot->bitmap_top;
        info.advance_x = (float)slot->advance.x / 64.0f;
        info.advance_y = (float)slot->advance.y / 64.0f;
        info.width = (float)gw;
        info.height = (float)gh;

        glyph_cache[codepoint] = info;

        atlas_cursor_x += gw + 1;
        atlas_row_height = std::max(atlas_row_height, gh);
    }
};

TextEngine::TextEngine() : m_impl(std::make_unique<Impl>()) {}
TextEngine::~TextEngine() = default;

bool TextEngine::load_font(const std::string& path, float size_pt) {
    m_size_pt = size_pt;
    if (!m_impl->ft_library) {
        if (FT_Init_FreeType(&m_impl->ft_library)) return false;
    }
    if (FT_New_Face(m_impl->ft_library, path.c_str(), 0, &m_impl->ft_face)) return false;
    FT_Set_Char_Size(m_impl->ft_face, 0, (int)(size_pt * 64), 72, 72);

    m_impl->hb_font = hb_ft_font_create_referenced(m_impl->ft_face);
    m_impl->loaded = true;
    m_impl->cache_all_ascii();
    return true;
}

bool TextEngine::load_font_from_memory(const uint8_t* data, size_t size, float size_pt) {
    m_size_pt = size_pt;
    if (!m_impl->ft_library) {
        if (FT_Init_FreeType(&m_impl->ft_library)) return false;
    }
    if (FT_New_Memory_Face(m_impl->ft_library, data, (FT_Long)size, 0, &m_impl->ft_face)) return false;
    FT_Set_Char_Size(m_impl->ft_face, 0, (int)(size_pt * 64), 72, 72);

    m_impl->hb_font = hb_ft_font_create_referenced(m_impl->ft_face);
    m_impl->loaded = true;
    m_impl->cache_all_ascii();
    return true;
}

void TextEngine::set_point_size(float pt) {
    m_size_pt = pt;
    if (m_impl->ft_face) {
        FT_Set_Char_Size(m_impl->ft_face, 0, (int)(pt * 64), 72, 72);
        m_impl->glyph_cache.clear();
        m_impl->atlas_buffer.clear();
        m_impl->atlas_cursor_x = 1;
        m_impl->atlas_cursor_y = 1;
        m_impl->cache_all_ascii();
    }
}

FontMetrics TextEngine::metrics() const {
    FontMetrics m{};
    if (!m_impl->ft_face) return m;
    FT_Face face = m_impl->ft_face;
    m.ascender = (float)face->size->metrics.ascender / 64.0f;
    m.descender = (float)face->size->metrics.descender / 64.0f;
    m.line_height = (float)face->size->metrics.height / 64.0f;
    m.underline_position = (float)face->underline_position / 64.0f;
    m.underline_thickness = (float)face->underline_thickness / 64.0f;
    m.units_per_em = face->units_per_EM;
    return m;
}

GlyphInfo TextEngine::get_glyph(uint32_t cp) const {
    const_cast<TextEngine*>(this)->m_impl->rasterize_glyph(cp);
    auto it = m_impl->glyph_cache.find(cp);
    if (it != m_impl->glyph_cache.end()) return it->second;
    return {};
}

bool TextEngine::has_glyph(uint32_t cp) const {
    return m_impl->glyph_cache.count(cp) > 0;
}

TextLayout TextEngine::layout_text(const std::string& text, float max_width) {
    TextLayout layout;
    layout.text = text;
    if (!m_impl->loaded) return layout;

    float x = 0, y = 0;

    for (size_t i = 0; i < text.size();) {
        uint32_t cp;
        int len = 1;
        uint8_t c = (uint8_t)text[i];
        if (c < 0x80) { cp = c; len = 1; }
        else if (c < 0xE0) { cp = ((c & 0x1F) << 6) | (text[i+1] & 0x3F); len = 2; }
        else if (c < 0xF0) { cp = ((c & 0x0F) << 12) | ((text[i+1] & 0x3F) << 6) | (text[i+2] & 0x3F); len = 3; }
        else { cp = ((c & 0x07) << 18) | ((text[i+1] & 0x3F) << 12) | ((text[i+2] & 0x3F) << 6) | (text[i+3] & 0x3F); len = 4; }

        if (cp == '\n') { x = 0; y += metrics().line_height; i += len; continue; }

        GlyphInfo gi = get_glyph(cp);

        if (max_width > 0 && x + gi.advance_x > max_width && i > 0) {
            x = 0;
            y += metrics().line_height;
        }

        layout.positions.push_back({(uint32_t)layout.positions.size(), x + gi.bearing_x, y + (metrics().ascender - gi.bearing_y), cp});
        x += gi.advance_x;
        i += len;
    }

    layout.total_width = x;
    layout.total_height = y + metrics().line_height;
    layout.baseline = metrics().ascender;

    return layout;
}

float TextEngine::measure_text(const std::string& text) {
    return layout_text(text).total_width;
}

TextEngine::AtlasEntry TextEngine::get_atlas() const {
    AtlasEntry e;
    e.data = m_impl->atlas_buffer.empty() ? nullptr : m_impl->atlas_buffer.data();
    e.width = m_impl->atlas_width;
    e.height = m_impl->atlas_height;
    return e;
}

} // namespace ovui
