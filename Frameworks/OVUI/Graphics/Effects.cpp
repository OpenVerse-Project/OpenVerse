#include "Effects.h"
#include <cstring>
#include <algorithm>

namespace ovui {

EffectsEngine::EffectsEngine() {}

void EffectsEngine::ensure_temp(size_t size) {
    if (m_temp_buffer.size() < size) m_temp_buffer.resize(size);
}

float EffectsEngine::gaussian(float x, float sigma) {
    float c = 1.0f / (sigma * 2.506628f);
    return c * std::exp(-(x * x) / (2.0f * sigma * sigma));
}

std::vector<float> EffectsEngine::build_kernel(int radius, float sigma) {
    std::vector<float> k(radius * 2 + 1);
    float sum = 0;
    for (int i = 0; i <= radius * 2; i++) {
        k[i] = gaussian((float)(i - radius), sigma);
        sum += k[i];
    }
    for (int i = 0; i <= radius * 2; i++) k[i] /= sum;
    return k;
}

void EffectsEngine::blur_horizontal(float* dst, const float* src, int w, int h, int radius, float sigma) {
    auto kernel = build_kernel(radius, sigma);
    int ks = radius * 2 + 1;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float r = 0, g = 0, b = 0, a = 0;
            for (int k = 0; k < ks; k++) {
                int sx = x + k - radius;
                if (sx < 0) sx = 0;
                if (sx >= w) sx = w - 1;
                int idx = (y * w + sx) * 4;
                float wk = kernel[k];
                r += src[idx] * wk; g += src[idx+1] * wk;
                b += src[idx+2] * wk; a += src[idx+3] * wk;
            }
            int o = (y * w + x) * 4;
            dst[o] = r; dst[o+1] = g; dst[o+2] = b; dst[o+3] = a;
        }
    }
}

void EffectsEngine::blur_vertical(float* dst, const float* src, int w, int h, int radius, float sigma) {
    auto kernel = build_kernel(radius, sigma);
    int ks = radius * 2 + 1;
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            float r = 0, g = 0, b = 0, a = 0;
            for (int k = 0; k < ks; k++) {
                int sy = y + k - radius;
                if (sy < 0) sy = 0;
                if (sy >= h) sy = h - 1;
                int idx = (sy * w + x) * 4;
                float wk = kernel[k];
                r += src[idx] * wk; g += src[idx+1] * wk;
                b += src[idx+2] * wk; a += src[idx+3] * wk;
            }
            int o = (y * w + x) * 4;
            dst[o] = r; dst[o+1] = g; dst[o+2] = b; dst[o+3] = a;
        }
    }
}

void EffectsEngine::apply_gaussian_blur(std::vector<float>& pixels, int w, int h, const BlurDesc& desc) {
    size_t sz = (size_t)w * h * 4;
    ensure_temp(sz);

    for (int pass = 0; pass < desc.passes; pass++) {
        blur_horizontal(m_temp_buffer.data(), pixels.data(), w, h, (int)desc.radius, desc.radius * 0.5f);
        blur_vertical(pixels.data(), m_temp_buffer.data(), w, h, (int)desc.radius, desc.radius * 0.5f);
    }
}

void EffectsEngine::apply_box_shadow(
    std::vector<float>& pixels, int w, int h,
    const Rect& rect, const BoxShadow& shadow, float radius)
{
    int rx = std::max(0, (int)rect.x - (int)shadow.blur_radius - (int)std::abs(shadow.offset_x) - 2);
    int ry = std::max(0, (int)rect.y - (int)shadow.blur_radius - (int)std::abs(shadow.offset_y) - 2);
    int rw = std::min((int)(rect.width + shadow.blur_radius * 2 + std::abs(shadow.offset_x) + 4), w - rx);
    int rh = std::min((int)(rect.height + shadow.blur_radius * 2 + std::abs(shadow.offset_y) + 4), h - ry);

    for (int y = ry; y < ry + rh && y < h; y++) {
        for (int x = rx; x < rx + rw && x < w; x++) {
            float dist_x = 0;
            if (x < rect.x + shadow.offset_x) dist_x = (rect.x + shadow.offset_x) - x;
            else if (x > rect.x + rect.width + shadow.offset_x) dist_x = x - (rect.x + rect.width + shadow.offset_x);

            float dist_y = 0;
            if (y < rect.y + shadow.offset_y) dist_y = (rect.y + shadow.offset_y) - y;
            else if (y > rect.y + rect.height + shadow.offset_y) dist_y = y - (rect.y + rect.height + shadow.offset_y);

            float dist = std::sqrt(dist_x * dist_x + dist_y * dist_y);
            float alpha = 1.0f - std::min(dist / (shadow.blur_radius + 1.0f), 1.0f);
            alpha *= alpha;

            int idx = (y * w + x) * 4;
            pixels[idx] = pixels[idx] * (1.0f - shadow.color.a * alpha) + shadow.color.r * shadow.color.a * alpha;
            pixels[idx+1] = pixels[idx+1] * (1.0f - shadow.color.a * alpha) + shadow.color.g * shadow.color.a * alpha;
            pixels[idx+2] = pixels[idx+2] * (1.0f - shadow.color.a * alpha) + shadow.color.b * shadow.color.a * alpha;
            pixels[idx+3] = 1.0f;
        }
    }
}

void EffectsEngine::apply_text_shadow(
    std::vector<float>& pixels, int w, int h,
    const std::string& text, const TextShadow& shadow,
    float font_size, Point baseline)
{
    float text_w = text.size() * font_size * 0.6f;
    float text_h = font_size;
    Rect text_rect{baseline.x, baseline.y - text_h, text_w, text_h};
    BoxShadow bs{shadow.offset_x, shadow.offset_y, shadow.blur_radius, 0, shadow.color, false};
    apply_box_shadow(pixels, w, h, text_rect, bs, 0);
}

void EffectsEngine::apply_glow(
    std::vector<float>& pixels, int w, int h,
    const Rect& region, Color glow_color, float radius)
{
    for (int y = std::max(0, (int)(region.y - radius * 2)); y < std::min(h, (int)(region.bottom() + radius * 2)); y++) {
        for (int x = std::max(0, (int)(region.x - radius * 2)); x < std::min(w, (int)(region.right() + radius * 2)); x++) {
            float dx = 0, dy = 0;
            if (x < region.x) dx = region.x - x;
            else if (x > region.right()) dx = x - region.right();
            if (y < region.y) dy = region.y - y;
            else if (y > region.bottom()) dy = y - region.bottom();

            float dist = std::sqrt(dx * dx + dy * dy) / (radius + 1.0f);
            float alpha = std::exp(-dist * dist) * glow_color.a;
            alpha = std::min(alpha, 1.0f);

            int idx = (y * w + x) * 4;
            pixels[idx] = pixels[idx] + glow_color.r * alpha;
            pixels[idx+1] = pixels[idx+1] + glow_color.g * alpha;
            pixels[idx+2] = pixels[idx+2] + glow_color.b * alpha;
            pixels[idx+3] = 1.0f;
        }
    }
}

void EffectsEngine::apply_glass_overlay(
    std::vector<float>& pixels, int w, int h,
    const Rect& region, float blur_amount, float opacity)
{
    size_t sz = (size_t)w * h * 4;
    std::vector<float> copy = pixels;

    BlurDesc bd{blur_amount, 1};
    apply_gaussian_blur(copy, w, h, bd);

    for (int y = std::max(0, (int)region.y); y < std::min(h, (int)region.bottom()); y++) {
        for (int x = std::max(0, (int)region.x); x < std::min(w, (int)region.right()); x++) {
            int idx = (y * w + x) * 4;
            for (int c = 0; c < 4; c++) {
                pixels[idx + c] = pixels[idx + c] * (1.0f - opacity) + copy[idx + c] * opacity;
            }
        }
    }
}

} // namespace ovui
