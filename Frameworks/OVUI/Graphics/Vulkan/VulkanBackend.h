#pragma once

#include <Graphics/RenderBackend.h>
#include <vector>
#include <memory>
#include <vulkan/vulkan.h>
#include <xcb/xcb.h>

namespace ovui {

struct VulkanBackendConfig {
    bool enable_validation = false;
    int preferred_gpu = 0;
};

class VulkanRenderBackend : public RenderBackend {
public:
    VulkanRenderBackend();
    ~VulkanRenderBackend() override;

    bool initialize(void* nw = nullptr) override;
    void shutdown() override;
    void begin_frame() override;
    void end_frame() override;
    void present() override;
    void execute_commands(const std::vector<RenderDrawCmd>& cmds) override;
    void resize(int w, int h) override;

    RenderBackendType type() const override { return RenderBackendType::Vulkan; }
    RenderBackendCapabilities capabilities() const override;
    RenderStats stats() const override;
    const char* name() const override { return "OVUI Vulkan"; }

    void set_config(const VulkanBackendConfig& c) { m_config = c; }

    bool is_running() const { return m_running; }
    void poll_events();
    int framebuffer_width() const { return m_w; }
    int framebuffer_height() const { return m_h; }

private:
    VulkanBackendConfig m_config;
    bool m_running = false;

    int m_w = 1280, m_h = 720;

    xcb_connection_t* m_conn = nullptr;
    xcb_window_t m_window = 0;
    xcb_atom_t m_wm_delete = 0;

    VkInstance m_instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT m_debug_messenger = VK_NULL_HANDLE;
    VkPhysicalDevice m_phys_dev = VK_NULL_HANDLE;
    VkDevice m_dev = VK_NULL_HANDLE;
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;

    uint32_t m_graphics_family = 0;
    uint32_t m_present_family = 0;
    VkQueue m_graphics_queue = VK_NULL_HANDLE;
    VkQueue m_present_queue = VK_NULL_HANDLE;

    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    VkFormat m_swapchain_format = VK_FORMAT_B8G8R8A8_UNORM;
    VkExtent2D m_swapchain_extent = {};
    std::vector<VkImage> m_swapchain_images;
    std::vector<VkImageView> m_swapchain_views;

    VkRenderPass m_render_pass = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> m_framebuffers;
    VkPipelineLayout m_pipeline_layout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;

    VkCommandPool m_cmd_pool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> m_cmd_bufs;

    static constexpr int MAX_FRAMES = 2;
    struct PerFrame {
        VkFence fence = VK_NULL_HANDLE;
        VkSemaphore image_available = VK_NULL_HANDLE;
        VkSemaphore render_finished = VK_NULL_HANDLE;

        VkBuffer vbuf = VK_NULL_HANDLE;
        VkDeviceMemory vbuf_mem = VK_NULL_HANDLE;
        VkDeviceSize vbuf_capacity = 0;
        void* vbuf_ptr = nullptr;

        VkBuffer ibuf = VK_NULL_HANDLE;
        VkDeviceMemory ibuf_mem = VK_NULL_HANDLE;
        VkDeviceSize ibuf_capacity = 0;
        void* ibuf_ptr = nullptr;
    };
    PerFrame m_frames[MAX_FRAMES];
    uint32_t m_frame_idx = 0;

    struct DrawBatch {
        uint32_t first_index = 0;
        uint32_t index_count = 0;
    };
    std::vector<DrawBatch> m_batches;

    bool create_instance();
    bool create_surface();
    bool pick_physical_device();
    bool create_logical_device();
    bool create_swapchain();
    bool create_render_pass();
    bool create_framebuffers();
    bool create_pipeline();
    bool create_command_pool();
    bool create_sync_objects();
    bool create_vertex_buffers();
    bool allocate_command_buffers();

    void cleanup_swapchain();
    void recreate_swapchain();
    void record_commands(VkCommandBuffer cmd, uint32_t image_idx);

    void tessellate_rect(std::vector<float>& verts, std::vector<uint16_t>& indices,
                         float x, float y, float w, float h, Color c, float radius);
    void tessellate_border(std::vector<float>& verts, std::vector<uint16_t>& indices,
                           float x, float y, float w, float h, Color c, float bw, float radius);
    void tessellate_circle(std::vector<float>& verts, std::vector<uint16_t>& indices,
                           float cx, float cy, float r, Color c, int segments = 32);
    void tessellate_line(std::vector<float>& verts, std::vector<uint16_t>& indices,
                         float x0, float y0, float x1, float y1, Color c, float width);
    void tessellate_text(std::vector<float>& verts, std::vector<uint16_t>& indices,
                         const std::string& text, float x, float y, Color c, float size);

    void emit_quad(std::vector<float>& verts, std::vector<uint16_t>& indices,
                   float x, float y, float w, float h, Color c);

    VkShaderModule load_shader(const uint32_t* code, size_t size);
    uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags props);
    void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                       VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem);
};

void register_vulkan_backend();

} // namespace ovui
