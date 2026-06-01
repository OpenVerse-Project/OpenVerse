#pragma once

#include <Core/Types.h>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <unordered_map>

namespace ovui {

class Widget;
class PaintContext;

struct RenderDrawCmd {
    enum Type { RectCmd, TextCmd, BorderCmd, CircleCmd, LineCmd } type;
    Rect frame;
    Color color;
    std::string text;
    float radius_or_size = 0, border_width = 1;
    Point p1, p2;
};

std::vector<RenderDrawCmd> convert_paint_commands(const PaintContext& ctx);

enum class RenderBackendType { CPU, Vulkan, DirectX12, Metal, OpenGL, Null };

struct RenderBackendCapabilities {
    bool gpu_accelerated = false;
    bool bindless_textures = false;
    bool compute_shaders = false;
    int max_texture_size = 4096;
    int max_draw_calls = 65536;
    std::string renderer_name;
    std::string driver_version;
};

struct RenderStats {
    int draw_calls = 0, triangles = 0, textures_bound = 0;
    float frame_time_ms = 0, gpu_time_ms = 0;
};

class RenderBackend {
public:
    virtual ~RenderBackend() = default;
    virtual bool initialize(void* nw = nullptr) = 0;
    virtual void shutdown() = 0;
    virtual void begin_frame() = 0;
    virtual void end_frame() = 0;
    virtual void present() = 0;
    virtual void execute_commands(const std::vector<RenderDrawCmd>& cmds) = 0;
    virtual void resize(int w, int h) = 0;
    virtual RenderBackendType type() const = 0;
    virtual RenderBackendCapabilities capabilities() const = 0;
    virtual RenderStats stats() const = 0;
    virtual const char* name() const = 0;
};

class CPURenderBackend : public RenderBackend {
public:
    CPURenderBackend();
    bool initialize(void* nw = nullptr) override;
    void shutdown() override;
    void begin_frame() override;
    void end_frame() override;
    void present() override;
    void execute_commands(const std::vector<RenderDrawCmd>& cmds) override;
    void resize(int w, int h) override;
    RenderBackendType type() const override { return RenderBackendType::CPU; }
    RenderBackendCapabilities capabilities() const override;
    RenderStats stats() const override;
    const char* name() const override { return "OVUI CPU Renderer"; }
    const std::vector<uint32_t>& framebuffer() const { return m_fb; }
    int fb_width() const { return m_w; }
    int fb_height() const { return m_h; }
    bool save_ppm(const std::string& path) const;
private:
    std::vector<uint32_t> m_fb;
    int m_w = 800, m_h = 600;
    RenderStats m_stats;
    void rect(int x,int y,int w,int h,Color c,float r);
    void text(const std::string& t,int x,int y,Color c,float s);
    void border(int x,int y,int w,int h,Color c,float bw,float r);
    void circle(int cx,int cy,int r,Color c);
};

class RenderBackendRegistry {
public:
    static RenderBackendRegistry& instance();
    void register_backend(const std::string& n, std::function<std::unique_ptr<RenderBackend>()> f);
    std::unique_ptr<RenderBackend> create(const std::string& n) const;
    std::unique_ptr<RenderBackend> create_default() const;
    std::vector<std::string> available_backends() const;
private:
    std::unordered_map<std::string, std::function<std::unique_ptr<RenderBackend>()>> m_factories;
    RenderBackendRegistry();
};

class Renderer {
public:
    Renderer(); ~Renderer();
    void set_backend(std::unique_ptr<RenderBackend> b);
    RenderBackend* backend() const { return m_backend.get(); }
    bool initialize(int w, int h, void* nw = nullptr);
    void shutdown();
    void begin_frame();
    void render(const std::vector<RenderDrawCmd>& cmds);
    void end_frame();
    void present();
    void resize(int w, int h);
    RenderStats stats() const;
private:
    std::unique_ptr<RenderBackend> m_backend;
    bool m_init = false;
};

} // namespace ovui
