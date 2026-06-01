#pragma once

#include <Core/Types.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>

typedef struct FT_FaceRec_* FT_Face;

namespace ovui {

struct GlyphInfo {
    uint32_t codepoint;
    uint32_t texture_id;
    float atlas_u, atlas_v;
    float atlas_w, atlas_h;
    float bearing_x, bearing_y;
    float advance_x, advance_y;
    float width, height;
};

struct TextLayout {
    std::string text;
    float total_width = 0;
    float total_height = 0;
    float baseline = 0;
    struct GlyphPos {
        uint32_t glyph_index;
        float x, y;
        uint32_t codepoint;
    };
    std::vector<GlyphPos> positions;
};

struct FontMetrics {
    float ascender = 0;
    float descender = 0;
    float line_height = 0;
    float underline_position = 0;
    float underline_thickness = 0;
    int units_per_em = 0;
};

class TextEngine {
public:
    TextEngine();
    ~TextEngine();

    bool load_font(const std::string& path, float size_pt = 14.0f);
    bool load_font_from_memory(const uint8_t* data, size_t size, float size_pt = 14.0f);

    FontMetrics metrics() const;
    GlyphInfo get_glyph(uint32_t codepoint) const;
    bool has_glyph(uint32_t codepoint) const;

    TextLayout layout_text(const std::string& text, float max_width = 0);
    float measure_text(const std::string& text);

    float point_size() const { return m_size_pt; }
    void set_point_size(float pt);

    struct AtlasEntry {
        uint8_t* data = nullptr;
        int width = 0, height = 0;
    };
    AtlasEntry get_atlas() const;

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    float m_size_pt = 14.0f;
};

} // namespace ovui
