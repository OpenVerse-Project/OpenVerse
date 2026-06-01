#include "RenderBackend.h"
#include <Widgets/Widgets.h>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <cstdint>
#include <fstream>

namespace ovui {

static uint32_t color_to_rgba(Color c) {
    uint8_t r = static_cast<uint8_t>(std::clamp(c.r, 0.0f, 1.0f) * 255.0f);
    uint8_t g = static_cast<uint8_t>(std::clamp(c.g, 0.0f, 1.0f) * 255.0f);
    uint8_t b = static_cast<uint8_t>(std::clamp(c.b, 0.0f, 1.0f) * 255.0f);
    uint8_t a = static_cast<uint8_t>(std::clamp(c.a, 0.0f, 1.0f) * 255.0f);
    return (static_cast<uint32_t>(a) << 24) | (static_cast<uint32_t>(b) << 16) |
           (static_cast<uint32_t>(g) << 8) | static_cast<uint32_t>(r);
}

static uint32_t blend_pixel(uint32_t dst, uint32_t src) {
    uint32_t sa = src >> 24;
    if (sa == 0) return dst;
    if (sa == 255) return src;
    uint32_t da = dst >> 24;
    uint32_t inv_sa = 255 - sa;
    uint32_t r = (((src & 0xFF) * sa) + ((dst & 0xFF) * inv_sa)) / 255;
    uint32_t g = ((((src >> 8) & 0xFF) * sa) + (((dst >> 8) & 0xFF) * inv_sa)) / 255;
    uint32_t b = ((((src >> 16) & 0xFF) * sa) + (((dst >> 16) & 0xFF) * inv_sa)) / 255;
    uint32_t a = std::max(sa, da);
    return (a << 24) | (b << 16) | (g << 8) | r;
}

static inline void set_pixel(std::vector<uint32_t>& fb, int w, int h, int x, int y, uint32_t col) {
    if (x < 0 || x >= w || y < 0 || y >= h) return;
    size_t idx = static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x);
    if (idx >= fb.size()) return;
    fb[idx] = blend_pixel(fb[idx], col);
}

CPURenderBackend::CPURenderBackend() {}
bool CPURenderBackend::initialize(void*) {
    if (m_w <= 0) m_w = 1;
    if (m_h <= 0) m_h = 1;
    size_t sz = static_cast<size_t>(m_w) * static_cast<size_t>(m_h);
    if (sz > 256 * 1024 * 1024) sz = 256 * 1024 * 1024;
    m_fb.resize(sz, 0xFF1E1E2E);
    return true;
}
void CPURenderBackend::shutdown() { m_fb.clear(); m_fb.shrink_to_fit(); }
void CPURenderBackend::begin_frame() { m_stats = {}; }
void CPURenderBackend::end_frame() {}
void CPURenderBackend::present() {}

void CPURenderBackend::resize(int w, int h) {
    m_w = std::max(1, std::min(w, 16384));
    m_h = std::max(1, std::min(h, 16384));
    size_t sz = static_cast<size_t>(m_w) * static_cast<size_t>(m_h);
    m_fb.resize(sz, 0xFF1E1E2E);
}

RenderBackendCapabilities CPURenderBackend::capabilities() const {
    return {false, false, false, 8192, 65536, "OVUI CPU", "1.0"};
}
RenderStats CPURenderBackend::stats() const { return m_stats; }

void CPURenderBackend::rect(int x, int y, int w, int h, Color c, float radius) {
    if (w <= 0 || h <= 0) return;
    uint32_t col = color_to_rgba(c);
    int ir = static_cast<int>(std::max(0.0f, radius));
    int x0 = x, y0 = y, x1 = x + w, y1 = y + h;

    for (int py = y0; py < y1; py++) {
        for (int px = x0; px < x1; px++) {
            if (ir > 0) {
                int cd_x = 0;
                if (px < x0 + ir) cd_x = x0 + ir - px;
                else if (px > x1 - ir - 1) cd_x = px - (x1 - ir - 1);

                int cd_y = 0;
                if (py < y0 + ir) cd_y = y0 + ir - py;
                else if (py > y1 - ir - 1) cd_y = py - (y1 - ir - 1);

                if (cd_x > 0 || cd_y > 0) {
                    float dist = std::sqrt(static_cast<float>(cd_x * cd_x + cd_y * cd_y));
                    if (dist > static_cast<float>(ir)) continue;

                    float edge_alpha = c.a;
                    if (dist > static_cast<float>(ir) - 1.0f)
                        edge_alpha *= std::max(0.0f, static_cast<float>(ir) - dist);
                    uint32_t soft_col = color_to_rgba({c.r, c.g, c.b, std::clamp(edge_alpha, 0.0f, 1.0f)});
                    set_pixel(m_fb, m_w, m_h, px, py, soft_col);
                    continue;
                }
            }
            set_pixel(m_fb, m_w, m_h, px, py, col);
        }
    }
}

void CPURenderBackend::text(const std::string& t, int x, int y, Color c, float size) {
    if (size <= 0 || t.empty()) return;
    int char_w = static_cast<int>(size * 0.6f);
    int char_h = static_cast<int>(size);
    if (char_w <= 0) char_w = 1;
    if (char_h <= 0) char_h = 1;
    uint32_t col = color_to_rgba(c);

    for (size_t ci = 0; ci < t.size(); ci++) {
        int cx = x + static_cast<int>(ci) * char_w;
        for (int py = y; py < y + char_h; py++) {
            for (int px = cx; px < cx + char_w; px++) {
                if ((px + py) % 4 == 0)
                    set_pixel(m_fb, m_w, m_h, px, py, col);
            }
        }
    }
}

void CPURenderBackend::border(int x, int y, int w, int h, Color c, float bw, float) {
    if (w <= 0 || h <= 0) return;
    int ibw = std::max(1, static_cast<int>(bw));
    rect(x, y, w, ibw, c, 0);
    rect(x, y + h - ibw, w, ibw, c, 0);
    rect(x, y + ibw, ibw, h - ibw * 2, c, 0);
    rect(x + w - ibw, y + ibw, ibw, h - ibw * 2, c, 0);
}

void CPURenderBackend::circle(int cx, int cy, int r, Color c) {
    if (r <= 0) return;
    uint32_t col = color_to_rgba(c);
    int r2 = r * r;
    for (int py = cy - r; py <= cy + r; py++) {
        for (int px = cx - r; px <= cx + r; px++) {
            int dx = px - cx, dy = py - cy;
            if (dx * dx + dy * dy <= r2)
                set_pixel(m_fb, m_w, m_h, px, py, col);
        }
    }
}

void CPURenderBackend::execute_commands(const std::vector<RenderDrawCmd>& cmds) {
    for (auto& cmd : cmds) {
        int x = static_cast<int>(cmd.frame.x);
        int y = static_cast<int>(cmd.frame.y);
        int w = static_cast<int>(cmd.frame.width);
        int h = static_cast<int>(cmd.frame.height);

        if (x > m_w || y > m_h) continue;

        switch (cmd.type) {
            case RenderDrawCmd::RectCmd:
                rect(x, y, std::max(w, 1), std::max(h, 1), cmd.color, std::max(0.0f, cmd.radius_or_size));
                break;
            case RenderDrawCmd::TextCmd:
                text(cmd.text, x, y, cmd.color, std::max(1.0f, cmd.radius_or_size));
                break;
            case RenderDrawCmd::BorderCmd:
                border(x, y, std::max(w, 1), std::max(h, 1), cmd.color,
                       std::max(1.0f, cmd.border_width), std::max(0.0f, cmd.radius_or_size));
                break;
            case RenderDrawCmd::CircleCmd:
                circle(x + w / 2, y + w / 2, w / 2, cmd.color);
                break;
            case RenderDrawCmd::LineCmd: {
                int lx0 = static_cast<int>(cmd.p1.x);
                int ly0 = static_cast<int>(cmd.p1.y);
                int lx1 = static_cast<int>(cmd.p2.x);
                int ly1 = static_cast<int>(cmd.p2.y);
                float bw = std::max(1.0f, cmd.border_width);
                int dx = std::abs(lx1 - lx0), dy = std::abs(ly1 - ly0);
                int sx = lx0 < lx1 ? 1 : -1, sy = ly0 < ly1 ? 1 : -1;
                int err = dx - dy;
                uint32_t col = color_to_rgba(cmd.color);
                int half = static_cast<int>(bw * 0.5f);
                while (true) {
                    for (int oy = -half; oy <= half; oy++)
                        for (int ox = -half; ox <= half; ox++)
                            set_pixel(m_fb, m_w, m_h, lx0 + ox, ly0 + oy, col);
                    if (lx0 == lx1 && ly0 == ly1) break;
                    int e2 = err * 2;
                    if (e2 > -dy) { err -= dy; lx0 += sx; }
                    if (e2 < dx) { err += dx; ly0 += sy; }
                }
                break;
            }
        }
        m_stats.draw_calls++;
    }
}

// ============================================================
// RenderBackendRegistry
// ============================================================
RenderBackendRegistry::RenderBackendRegistry() {
    register_backend("cpu", [](){return std::make_unique<CPURenderBackend>();});
}

RenderBackendRegistry& RenderBackendRegistry::instance() {
    static RenderBackendRegistry reg;
    return reg;
}

void RenderBackendRegistry::register_backend(const std::string& name, std::function<std::unique_ptr<RenderBackend>()> f) {
    m_factories[name] = std::move(f);
}

std::unique_ptr<RenderBackend> RenderBackendRegistry::create(const std::string& name) const {
    auto it = m_factories.find(name);
    return it != m_factories.end() ? it->second() : nullptr;
}

std::unique_ptr<RenderBackend> RenderBackendRegistry::create_default() const {
    return create("cpu");
}

std::vector<std::string> RenderBackendRegistry::available_backends() const {
    std::vector<std::string> names;
    for (auto& [k, _] : m_factories) names.push_back(k);
    return names;
}

// ============================================================
// Renderer
// ============================================================
Renderer::Renderer() {}
Renderer::~Renderer() { shutdown(); }

void Renderer::set_backend(std::unique_ptr<RenderBackend> b) { m_backend = std::move(b); }

bool Renderer::initialize(int w, int h, void* nw) {
    if (!m_backend) m_backend = RenderBackendRegistry::instance().create_default();
    if (!m_backend) return false;
    m_backend->resize(w, h);
    m_init = m_backend->initialize(nw);
    return m_init;
}

void Renderer::shutdown() {
    if (m_backend && m_init) {
        m_backend->shutdown();
        m_init = false;
    }
}
void Renderer::begin_frame() { if (m_backend) m_backend->begin_frame(); }

void Renderer::render(const std::vector<RenderDrawCmd>& cmds) {
    if (m_backend) m_backend->execute_commands(cmds);
}

void Renderer::end_frame() { if (m_backend) m_backend->end_frame(); }
void Renderer::present() { if (m_backend) m_backend->present(); }
void Renderer::resize(int w, int h) { if (m_backend) m_backend->resize(w, h); }
RenderStats Renderer::stats() const { return m_backend ? m_backend->stats() : RenderStats{}; }

bool CPURenderBackend::save_ppm(const std::string& path) const {
    std::ofstream f(path, std::ios::binary);
    if (!f) return false;
    f << "P6\n" << m_w << " " << m_h << "\n255\n";
    for (int y = 0; y < m_h; y++) {
        for (int x = 0; x < m_w; x++) {
            uint32_t p = m_fb[(size_t)y * (size_t)m_w + (size_t)x];
            uint8_t rgb[3] = {static_cast<uint8_t>(p & 0xFF),
                              static_cast<uint8_t>((p >> 8) & 0xFF),
                              static_cast<uint8_t>((p >> 16) & 0xFF)};
            f.write((const char*)rgb, 3);
        }
    }
    return f.good();
}

std::vector<RenderDrawCmd> convert_paint_commands(const PaintContext& ctx) {
    std::vector<RenderDrawCmd> out;
    for (auto& cmd : ctx.commands()) {
        RenderDrawCmd rdc;
        rdc.frame = cmd.frame;
        rdc.color = cmd.color;
        rdc.text = cmd.text;
        rdc.radius_or_size = cmd.radius_or_size;
        rdc.border_width = cmd.border_width;

        switch (cmd.type) {
            case PaintContext::DrawCmd::RectCmd:  rdc.type = RenderDrawCmd::RectCmd; break;
            case PaintContext::DrawCmd::TextCmd:  rdc.type = RenderDrawCmd::TextCmd; break;
            case PaintContext::DrawCmd::BorderCmd: rdc.type = RenderDrawCmd::BorderCmd; break;
            case PaintContext::DrawCmd::LineCmd:  rdc.type = RenderDrawCmd::LineCmd;
                rdc.p1 = {cmd.frame.x, cmd.frame.y};
                rdc.p2 = {cmd.frame.x + cmd.frame.width, cmd.frame.y + cmd.frame.height};
                rdc.border_width = cmd.border_width;
                break;
        }
        out.push_back(rdc);
    }
    return out;
}

} // namespace ovui
