#pragma once

#include <Core/Types.h>
#include <vector>
#include <cmath>

namespace ovui {

struct BoxShadow {
    float offset_x = 0, offset_y = 2;
    float blur_radius = 8;
    float spread = 0;
    Color color{0,0,0,0.3f};
    bool inset = false;
};

struct TextShadow {
    float offset_x = 1, offset_y = 1;
    float blur_radius = 3;
    Color color{0,0,0,0.5f};
};

struct BlurDesc {
    float radius = 4;
    int passes = 3;
};

class EffectsEngine {
public:
    EffectsEngine();

    void apply_box_shadow(
        std::vector<float>& pixels, int width, int height,
        const Rect& widget_rect, const BoxShadow& shadow, float corner_radius = 0
    );

    void apply_text_shadow(
        std::vector<float>& pixels, int width, int height,
        const std::string& text, const TextShadow& shadow,
        float font_size = 14, Point baseline = {0,0}
    );

    void apply_gaussian_blur(
        std::vector<float>& pixels, int width, int height,
        const BlurDesc& desc
    );

    void apply_glow(
        std::vector<float>& pixels, int width, int height,
        const Rect& region, Color glow_color, float radius = 10
    );

    void apply_glass_overlay(
        std::vector<float>& pixels, int width, int height,
        const Rect& region, float blur_amount = 5, float opacity = 0.3f
    );

    static float gaussian(float x, float sigma);
    static std::vector<float> build_kernel(int radius, float sigma);
    static void blur_horizontal(float* dst, const float* src, int w, int h, int radius, float sigma);
    static void blur_vertical(float* dst, const float* src, int w, int h, int radius, float sigma);

private:
    std::vector<float> m_temp_buffer;
    void ensure_temp(size_t size);
};

} // namespace ovui
