#include "VulkanBackend.h"
#include "ui_rect_vert_spv.h"
#include "ui_rect_frag_spv.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <vector>
#include <set>
#include <cassert>

namespace ovui {

static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* data,
    void*) {
    fprintf(stderr, "[VULKAN] %s\n", data->pMessage);
    return VK_FALSE;
}

static VkResult create_debug_messenger(VkInstance inst, const VkDebugUtilsMessengerCreateInfoEXT* ci,
                                        VkDebugUtilsMessengerEXT* msgr) {
    auto fn = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkCreateDebugUtilsMessengerEXT");
    return fn ? fn(inst, ci, nullptr, msgr) : VK_ERROR_EXTENSION_NOT_PRESENT;
}
static void destroy_debug_messenger(VkInstance inst, VkDebugUtilsMessengerEXT msgr) {
    auto fn = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(inst, "vkDestroyDebugUtilsMessengerEXT");
    if (fn) fn(inst, msgr, nullptr);
}

VulkanRenderBackend::VulkanRenderBackend() { memset(m_frames, 0, sizeof(m_frames)); }
VulkanRenderBackend::~VulkanRenderBackend() { shutdown(); }

bool VulkanRenderBackend::initialize(void*) {
    if (!create_instance()) return false;
    if (!create_surface()) return false;
    if (!pick_physical_device()) return false;
    if (!create_logical_device()) return false;
    if (!create_swapchain()) return false;
    if (!create_render_pass()) return false;
    if (!create_framebuffers()) return false;
    if (!create_pipeline()) return false;
    if (!create_command_pool()) return false;
    if (!create_vertex_buffers()) return false;
    if (!create_sync_objects()) return false;
    if (!allocate_command_buffers()) return false;
    m_running = true;
    return true;
}

void VulkanRenderBackend::shutdown() {
    if (!m_dev) return;

    if (m_dev) vkDeviceWaitIdle(m_dev);

    for (auto& f : m_frames) {
        if (f.fence) { vkDestroyFence(m_dev, f.fence, nullptr); f.fence = VK_NULL_HANDLE; }
        if (f.image_available) { vkDestroySemaphore(m_dev, f.image_available, nullptr); f.image_available = VK_NULL_HANDLE; }
        if (f.render_finished) { vkDestroySemaphore(m_dev, f.render_finished, nullptr); f.render_finished = VK_NULL_HANDLE; }
        if (f.vbuf) { vkDestroyBuffer(m_dev, f.vbuf, nullptr); f.vbuf = VK_NULL_HANDLE; }
        if (f.ibuf) { vkDestroyBuffer(m_dev, f.ibuf, nullptr); f.ibuf = VK_NULL_HANDLE; }
        if (f.vbuf_mem) { vkFreeMemory(m_dev, f.vbuf_mem, nullptr); f.vbuf_mem = VK_NULL_HANDLE; }
        if (f.ibuf_mem) { vkFreeMemory(m_dev, f.ibuf_mem, nullptr); f.ibuf_mem = VK_NULL_HANDLE; }
    }

    cleanup_swapchain();

    if (m_pipeline) { vkDestroyPipeline(m_dev, m_pipeline, nullptr); m_pipeline = VK_NULL_HANDLE; }
    if (m_pipeline_layout) { vkDestroyPipelineLayout(m_dev, m_pipeline_layout, nullptr); m_pipeline_layout = VK_NULL_HANDLE; }
    if (m_render_pass) { vkDestroyRenderPass(m_dev, m_render_pass, nullptr); m_render_pass = VK_NULL_HANDLE; }
    if (m_cmd_pool) { vkDestroyCommandPool(m_dev, m_cmd_pool, nullptr); m_cmd_pool = VK_NULL_HANDLE; }

    vkDestroyDevice(m_dev, nullptr);
    m_dev = VK_NULL_HANDLE;
    m_phys_dev = VK_NULL_HANDLE;

    if (m_surface) { vkDestroySurfaceKHR(m_instance, m_surface, nullptr); m_surface = VK_NULL_HANDLE; }
    if (m_debug_messenger) { destroy_debug_messenger(m_instance, m_debug_messenger); m_debug_messenger = VK_NULL_HANDLE; }
    if (m_instance) { vkDestroyInstance(m_instance, nullptr); m_instance = VK_NULL_HANDLE; }

    if (m_conn && m_window) {
        xcb_destroy_window(m_conn, m_window);
        xcb_disconnect(m_conn);
        m_conn = nullptr;
        m_window = 0;
    }
    m_running = false;
}

bool VulkanRenderBackend::create_instance() {
    m_conn = xcb_connect(nullptr, nullptr);
    if (xcb_connection_has_error(m_conn)) return false;

    VkApplicationInfo ai{};
    ai.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    ai.pApplicationName = "OVUI";
    ai.applicationVersion = VK_MAKE_VERSION(1,0,0);
    ai.pEngineName = "OpenVerse";
    ai.engineVersion = VK_MAKE_VERSION(2,7,0);
    ai.apiVersion = VK_API_VERSION_1_2;

    std::vector<const char*> exts = {VK_KHR_SURFACE_EXTENSION_NAME, VK_KHR_XCB_SURFACE_EXTENSION_NAME};
    if (m_config.enable_validation)
        exts.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    VkInstanceCreateInfo ci{};
    ci.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    ci.pApplicationInfo = &ai;
    ci.enabledExtensionCount = (uint32_t)exts.size();
    ci.ppEnabledExtensionNames = exts.data();

    const char* layers[] = {"VK_LAYER_KHRONOS_validation"};
    if (m_config.enable_validation) {
        ci.enabledLayerCount = 1;
        ci.ppEnabledLayerNames = layers;
    }

    if (vkCreateInstance(&ci, nullptr, &m_instance) != VK_SUCCESS) return false;

    if (m_config.enable_validation) {
        VkDebugUtilsMessengerCreateInfoEXT dci{};
        dci.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        dci.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                              VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        dci.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                          VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT;
        dci.pfnUserCallback = debug_callback;
        create_debug_messenger(m_instance, &dci, &m_debug_messenger);
    }
    return true;
}

bool VulkanRenderBackend::create_surface() {
    VkXcbSurfaceCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    sci.connection = m_conn;

    const xcb_setup_t* setup = xcb_get_setup(m_conn);
    xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
    xcb_screen_t* screen = iter.data;

    m_window = xcb_generate_id(m_conn);
    uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    uint32_t vals[] = {screen->white_pixel,
                       XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_STRUCTURE_NOTIFY};
    xcb_create_window(m_conn, XCB_COPY_FROM_PARENT, m_window, screen->root,
                      0, 0, m_w, m_h, 0,
                      XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, mask, vals);

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(m_conn, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t* reply = xcb_intern_atom_reply(m_conn, cookie, nullptr);
    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(m_conn, 0, 16, "WM_DELETE_WINDOW");
    xcb_intern_atom_reply_t* reply2 = xcb_intern_atom_reply(m_conn, cookie2, nullptr);
    if (reply && reply2) {
        m_wm_delete = reply2->atom;
        xcb_change_property(m_conn, XCB_PROP_MODE_REPLACE, m_window, reply->atom, 4, 32, 1, &reply2->atom);
    }
    free(reply); free(reply2);

    xcb_map_window(m_conn, m_window);
    xcb_flush(m_conn);

    sci.window = m_window;
    return vkCreateXcbSurfaceKHR(m_instance, &sci, nullptr, &m_surface) == VK_SUCCESS;
}

bool VulkanRenderBackend::pick_physical_device() {
    uint32_t count = 0;
    vkEnumeratePhysicalDevices(m_instance, &count, nullptr);
    if (count == 0) return false;
    std::vector<VkPhysicalDevice> devices(count);
    vkEnumeratePhysicalDevices(m_instance, &count, devices.data());

    m_phys_dev = devices[std::min((uint32_t)m_config.preferred_gpu, count - 1)];

    uint32_t qf_count = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_phys_dev, &qf_count, nullptr);
    std::vector<VkQueueFamilyProperties> qf_props(qf_count);
    vkGetPhysicalDeviceQueueFamilyProperties(m_phys_dev, &qf_count, qf_props.data());

    m_graphics_family = UINT32_MAX;
    m_present_family = UINT32_MAX;
    for (uint32_t i = 0; i < qf_count; i++) {
        if (qf_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) m_graphics_family = i;
        VkBool32 supports = VK_FALSE;
        vkGetPhysicalDeviceSurfaceSupportKHR(m_phys_dev, i, m_surface, &supports);
        if (supports) m_present_family = i;
    }
    return m_graphics_family != UINT32_MAX && m_present_family != UINT32_MAX;
}

bool VulkanRenderBackend::create_logical_device() {
    std::set<uint32_t> families = {m_graphics_family, m_present_family};
    std::vector<VkDeviceQueueCreateInfo> qcis;
    float prio = 1.0f;
    for (auto f : families) {
        VkDeviceQueueCreateInfo qci{};
        qci.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        qci.queueFamilyIndex = f;
        qci.queueCount = 1;
        qci.pQueuePriorities = &prio;
        qcis.push_back(qci);
    }

    std::vector<const char*> dev_exts = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    VkPhysicalDeviceFeatures feats{};

    VkDeviceCreateInfo dci{};
    dci.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    dci.queueCreateInfoCount = (uint32_t)qcis.size();
    dci.pQueueCreateInfos = qcis.data();
    dci.enabledExtensionCount = (uint32_t)dev_exts.size();
    dci.ppEnabledExtensionNames = dev_exts.data();
    dci.pEnabledFeatures = &feats;

    if (vkCreateDevice(m_phys_dev, &dci, nullptr, &m_dev) != VK_SUCCESS) return false;
    vkGetDeviceQueue(m_dev, m_graphics_family, 0, &m_graphics_queue);
    vkGetDeviceQueue(m_dev, m_present_family, 0, &m_present_queue);
    return true;
}

bool VulkanRenderBackend::create_swapchain() {
    VkSurfaceCapabilitiesKHR caps;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_phys_dev, m_surface, &caps);

    uint32_t fmt_count;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_phys_dev, m_surface, &fmt_count, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(fmt_count);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_phys_dev, m_surface, &fmt_count, formats.data());
    m_swapchain_format = formats[0].format;

    m_swapchain_extent = caps.currentExtent;
    if (m_swapchain_extent.width == UINT32_MAX)
        m_swapchain_extent = {(uint32_t)m_w, (uint32_t)m_h};

    uint32_t img_count = std::max(caps.minImageCount, 2u);
    if (caps.maxImageCount > 0)
        img_count = std::min(img_count, caps.maxImageCount);

    VkSwapchainCreateInfoKHR sci{};
    sci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    sci.surface = m_surface;
    sci.minImageCount = img_count;
    sci.imageFormat = m_swapchain_format;
    sci.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    sci.imageExtent = m_swapchain_extent;
    sci.imageArrayLayers = 1;
    sci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t families[] = {m_graphics_family, m_present_family};
    if (m_graphics_family != m_present_family) {
        sci.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        sci.queueFamilyIndexCount = 2;
        sci.pQueueFamilyIndices = families;
    } else {
        sci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    sci.preTransform = caps.currentTransform;
    sci.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    sci.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    sci.clipped = VK_TRUE;

    if (vkCreateSwapchainKHR(m_dev, &sci, nullptr, &m_swapchain) != VK_SUCCESS) return false;

    vkGetSwapchainImagesKHR(m_dev, m_swapchain, &img_count, nullptr);
    m_swapchain_images.resize(img_count);
    m_swapchain_views.resize(img_count);
    vkGetSwapchainImagesKHR(m_dev, m_swapchain, &img_count, m_swapchain_images.data());

    for (size_t i = 0; i < img_count; i++) {
        VkImageViewCreateInfo ivci{};
        ivci.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        ivci.image = m_swapchain_images[i];
        ivci.viewType = VK_IMAGE_VIEW_TYPE_2D;
        ivci.format = m_swapchain_format;
        ivci.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        ivci.subresourceRange.levelCount = 1;
        ivci.subresourceRange.layerCount = 1;
        vkCreateImageView(m_dev, &ivci, nullptr, &m_swapchain_views[i]);
    }
    return true;
}

bool VulkanRenderBackend::create_framebuffers() {
    m_framebuffers.resize(m_swapchain_views.size(), VK_NULL_HANDLE);
    for (size_t i = 0; i < m_swapchain_views.size(); i++) {
        VkImageView attachments[] = {m_swapchain_views[i]};
        VkFramebufferCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        fci.renderPass = m_render_pass;
        fci.attachmentCount = 1;
        fci.pAttachments = attachments;
        fci.width = m_swapchain_extent.width;
        fci.height = m_swapchain_extent.height;
        fci.layers = 1;
        if (vkCreateFramebuffer(m_dev, &fci, nullptr, &m_framebuffers[i]) != VK_SUCCESS)
            return false;
    }
    return true;
}

void VulkanRenderBackend::cleanup_swapchain() {
    for (auto& fb : m_framebuffers)
        if (fb) { vkDestroyFramebuffer(m_dev, fb, nullptr); fb = VK_NULL_HANDLE; }
    m_framebuffers.clear();
    for (auto& v : m_swapchain_views)
        if (v) { vkDestroyImageView(m_dev, v, nullptr); v = VK_NULL_HANDLE; }
    m_swapchain_views.clear();
    m_swapchain_images.clear();
    if (m_swapchain) { vkDestroySwapchainKHR(m_dev, m_swapchain, nullptr); m_swapchain = VK_NULL_HANDLE; }
}

void VulkanRenderBackend::recreate_swapchain() {
    if (!m_dev) return;
    vkDeviceWaitIdle(m_dev);
    cleanup_swapchain();
    create_swapchain();
    create_framebuffers();
}

bool VulkanRenderBackend::create_render_pass() {
    VkAttachmentDescription ca{};
    ca.format = m_swapchain_format;
    ca.samples = VK_SAMPLE_COUNT_1_BIT;
    ca.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    ca.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    ca.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    ca.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference car{0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL};

    VkSubpassDescription sp{};
    sp.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    sp.colorAttachmentCount = 1;
    sp.pColorAttachments = &car;

    VkRenderPassCreateInfo rpi{};
    rpi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    rpi.attachmentCount = 1;
    rpi.pAttachments = &ca;
    rpi.subpassCount = 1;
    rpi.pSubpasses = &sp;

    return vkCreateRenderPass(m_dev, &rpi, nullptr, &m_render_pass) == VK_SUCCESS;
}

VkShaderModule VulkanRenderBackend::load_shader(const uint32_t* code, size_t size) {
    VkShaderModuleCreateInfo smci{};
    smci.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    smci.codeSize = size;
    smci.pCode = code;
    VkShaderModule sm;
    vkCreateShaderModule(m_dev, &smci, nullptr, &sm);
    return sm;
}

bool VulkanRenderBackend::create_pipeline() {
    size_t vert_size = _home_dataline_openverse_Frameworks_OVUI_Shaders_ui_rect_vert_spv_len;
    size_t frag_size = _home_dataline_openverse_Frameworks_OVUI_Shaders_ui_rect_frag_spv_len;
    VkShaderModule vert_mod = load_shader((const uint32_t*)_home_dataline_openverse_Frameworks_OVUI_Shaders_ui_rect_vert_spv, vert_size);
    VkShaderModule frag_mod = load_shader((const uint32_t*)_home_dataline_openverse_Frameworks_OVUI_Shaders_ui_rect_frag_spv, frag_size);

    VkPipelineShaderStageCreateInfo vs{}, fs{};
    vs.sType = fs.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vs.stage = VK_SHADER_STAGE_VERTEX_BIT; vs.module = vert_mod; vs.pName = "main";
    fs.stage = VK_SHADER_STAGE_FRAGMENT_BIT; fs.module = frag_mod; fs.pName = "main";
    VkPipelineShaderStageCreateInfo stages[] = {vs, fs};

    VkVertexInputBindingDescription bind{};
    bind.binding = 0; bind.stride = 28; bind.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attrs[2]{};
    attrs[0].location = 0; attrs[0].binding = 0; attrs[0].format = VK_FORMAT_R32G32_SFLOAT; attrs[0].offset = 0;
    attrs[1].location = 1; attrs[1].binding = 0; attrs[1].format = VK_FORMAT_R32G32B32A32_SFLOAT; attrs[1].offset = 8;

    VkPipelineVertexInputStateCreateInfo vis{};
    vis.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vis.vertexBindingDescriptionCount = 1; vis.pVertexBindingDescriptions = &bind;
    vis.vertexAttributeDescriptionCount = 2; vis.pVertexAttributeDescriptions = attrs;

    VkPipelineInputAssemblyStateCreateInfo ias{};
    ias.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    ias.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

    VkViewport vp{0, 0, (float)m_swapchain_extent.width, (float)m_swapchain_extent.height, 0, 1};
    VkRect2D scissor{{0,0}, m_swapchain_extent};

    VkPipelineViewportStateCreateInfo vps{};
    vps.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    vps.viewportCount = 1; vps.pViewports = &vp;
    vps.scissorCount = 1; vps.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rs{};
    rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rs.lineWidth = 1.0f; rs.cullMode = VK_CULL_MODE_NONE;

    VkPipelineMultisampleStateCreateInfo ms{};
    ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineColorBlendAttachmentState cba{};
    cba.blendEnable = VK_TRUE;
    cba.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    cba.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    cba.colorBlendOp = VK_BLEND_OP_ADD;
    cba.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    cba.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    cba.alphaBlendOp = VK_BLEND_OP_ADD;
    cba.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
                         VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo cbs{};
    cbs.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    cbs.attachmentCount = 1; cbs.pAttachments = &cba;

    VkPushConstantRange pcr{VK_SHADER_STAGE_VERTEX_BIT, 0, 8};

    VkPipelineLayoutCreateInfo plci{};
    plci.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    plci.pushConstantRangeCount = 1; plci.pPushConstantRanges = &pcr;
    vkCreatePipelineLayout(m_dev, &plci, nullptr, &m_pipeline_layout);

    VkGraphicsPipelineCreateInfo pci{};
    pci.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pci.stageCount = 2; pci.pStages = stages;
    pci.pVertexInputState = &vis; pci.pInputAssemblyState = &ias;
    pci.pViewportState = &vps; pci.pRasterizationState = &rs;
    pci.pMultisampleState = &ms; pci.pColorBlendState = &cbs;
    pci.layout = m_pipeline_layout; pci.renderPass = m_render_pass;
    pci.subpass = 0;

    VkResult res = vkCreateGraphicsPipelines(m_dev, VK_NULL_HANDLE, 1, &pci, nullptr, &m_pipeline);
    vkDestroyShaderModule(m_dev, vert_mod, nullptr);
    vkDestroyShaderModule(m_dev, frag_mod, nullptr);
    return res == VK_SUCCESS;
}

bool VulkanRenderBackend::create_command_pool() {
    VkCommandPoolCreateInfo cpci{};
    cpci.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    cpci.queueFamilyIndex = m_graphics_family;
    cpci.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    return vkCreateCommandPool(m_dev, &cpci, nullptr, &m_cmd_pool) == VK_SUCCESS;
}

bool VulkanRenderBackend::allocate_command_buffers() {
    uint32_t count = (uint32_t)m_swapchain_images.size();
    m_cmd_bufs.resize(count);
    VkCommandBufferAllocateInfo cbai{};
    cbai.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    cbai.commandPool = m_cmd_pool;
    cbai.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    cbai.commandBufferCount = count;
    return vkAllocateCommandBuffers(m_dev, &cbai, m_cmd_bufs.data()) == VK_SUCCESS;
}

uint32_t VulkanRenderBackend::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags props) {
    VkPhysicalDeviceMemoryProperties mp;
    vkGetPhysicalDeviceMemoryProperties(m_phys_dev, &mp);
    for (uint32_t i = 0; i < mp.memoryTypeCount; i++)
        if ((type_filter & (1 << i)) && (mp.memoryTypes[i].propertyFlags & props) == props)
            return i;
    return 0;
}

void VulkanRenderBackend::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
                                         VkMemoryPropertyFlags props, VkBuffer& buf, VkDeviceMemory& mem) {
    VkBufferCreateInfo bci{};
    bci.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bci.size = size; bci.usage = usage; bci.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    if (vkCreateBuffer(m_dev, &bci, nullptr, &buf) != VK_SUCCESS) {
        buf = VK_NULL_HANDLE; mem = VK_NULL_HANDLE; return;
    }

    VkMemoryRequirements mr;
    vkGetBufferMemoryRequirements(m_dev, buf, &mr);

    VkMemoryAllocateInfo mai{};
    mai.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    mai.allocationSize = mr.size;
    mai.memoryTypeIndex = find_memory_type(mr.memoryTypeBits, props);
    if (vkAllocateMemory(m_dev, &mai, nullptr, &mem) != VK_SUCCESS) {
        vkDestroyBuffer(m_dev, buf, nullptr);
        buf = VK_NULL_HANDLE; mem = VK_NULL_HANDLE; return;
    }
    vkBindBufferMemory(m_dev, buf, mem, 0);
}

bool VulkanRenderBackend::create_vertex_buffers() {
    for (auto& f : m_frames) {
        VkDeviceSize vcap = 1024 * 1024 * 4;
        VkDeviceSize icap = 1024 * 1024 * 2;
        create_buffer(vcap, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      f.vbuf, f.vbuf_mem);
        create_buffer(icap, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      f.ibuf, f.ibuf_mem);
        f.vbuf_capacity = vcap; f.ibuf_capacity = icap;
        if (vkMapMemory(m_dev, f.vbuf_mem, 0, vcap, 0, &f.vbuf_ptr) != VK_SUCCESS)
            f.vbuf_ptr = nullptr;
        if (vkMapMemory(m_dev, f.ibuf_mem, 0, icap, 0, &f.ibuf_ptr) != VK_SUCCESS)
            f.ibuf_ptr = nullptr;
    }
    return true;
}

bool VulkanRenderBackend::create_sync_objects() {
    for (auto& f : m_frames) {
        VkFenceCreateInfo fci{};
        fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fci.flags = VK_FENCE_CREATE_SIGNALED_BIT;
        vkCreateFence(m_dev, &fci, nullptr, &f.fence);

        VkSemaphoreCreateInfo sci{};
        sci.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        vkCreateSemaphore(m_dev, &sci, nullptr, &f.image_available);
        vkCreateSemaphore(m_dev, &sci, nullptr, &f.render_finished);
    }
    return true;
}

void VulkanRenderBackend::emit_quad(std::vector<float>& verts, std::vector<uint16_t>& indices,
                                     float x, float y, float w, float h, Color c) {
    uint16_t base = (uint16_t)(verts.size() / 7);
    float rgba[4] = {c.r, c.g, c.b, c.a};
    verts.insert(verts.end(), {x, y, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    verts.insert(verts.end(), {x + w, y, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    verts.insert(verts.end(), {x + w, y + h, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    verts.insert(verts.end(), {x, y + h, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    indices.insert(indices.end(), {
        base, (uint16_t)(base + 1), (uint16_t)(base + 2),
        base, (uint16_t)(base + 2), (uint16_t)(base + 3)
    });
}

void VulkanRenderBackend::tessellate_rect(std::vector<float>& verts, std::vector<uint16_t>& indices,
                                           float x, float y, float w, float h, Color c, float) {
    emit_quad(verts, indices, x, y, w, h, c);
}

void VulkanRenderBackend::tessellate_border(std::vector<float>& verts, std::vector<uint16_t>& indices,
                                             float x, float y, float w, float h, Color c, float bw, float) {
    emit_quad(verts, indices, x, y, w, bw, c);
    emit_quad(verts, indices, x, y + h - bw, w, bw, c);
    emit_quad(verts, indices, x, y + bw, bw, h - bw * 2, c);
    emit_quad(verts, indices, x + w - bw, y + bw, bw, h - bw * 2, c);
}

void VulkanRenderBackend::tessellate_circle(std::vector<float>& verts, std::vector<uint16_t>& indices,
                                             float cx, float cy, float r, Color c, int segments) {
    float rgba[4] = {c.r, c.g, c.b, c.a};
    uint16_t center = (uint16_t)(verts.size() / 7);
    verts.insert(verts.end(), {cx, cy, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    for (int i = 0; i <= segments; i++) {
        float angle = 2.0f * 3.14159265f * i / segments;
        verts.insert(verts.end(), {cx + cosf(angle) * r, cy + sinf(angle) * r,
                                   rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    }
    for (int i = 0; i < segments; i++)
        indices.insert(indices.end(), {center, (uint16_t)(center + i + 1), (uint16_t)(center + i + 2)});
}

void VulkanRenderBackend::tessellate_line(std::vector<float>& verts, std::vector<uint16_t>& indices,
                                           float x0, float y0, float x1, float y1, Color c, float width) {
    float dx = x1 - x0, dy = y1 - y0;
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;
    float nx = -dy / len * width * 0.5f;
    float ny = dx / len * width * 0.5f;
    float rgba[4] = {c.r, c.g, c.b, c.a};
    uint16_t base = (uint16_t)(verts.size() / 7);
    verts.insert(verts.end(), {x0 + nx, y0 + ny, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    verts.insert(verts.end(), {x0 - nx, y0 - ny, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    verts.insert(verts.end(), {x1 - nx, y1 - ny, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    verts.insert(verts.end(), {x1 + nx, y1 + ny, rgba[0], rgba[1], rgba[2], rgba[3], 0.0f});
    indices.insert(indices.end(), {
        base, (uint16_t)(base + 1), (uint16_t)(base + 2),
        base, (uint16_t)(base + 2), (uint16_t)(base + 3)
    });
}

void VulkanRenderBackend::tessellate_text(std::vector<float>& verts, std::vector<uint16_t>& indices,
                                           const std::string& text, float x, float y, Color c, float size) {
    if (text.empty()) return;
    float char_w = size * 0.6f, char_h = size;
    for (size_t ci = 0; ci < text.size(); ci++)
        emit_quad(verts, indices, x + ci * char_w, y, char_w - 1, char_h, c);
}

void VulkanRenderBackend::begin_frame() {
    auto& f = m_frames[m_frame_idx];
    vkWaitForFences(m_dev, 1, &f.fence, VK_TRUE, UINT64_MAX);
    vkResetFences(m_dev, 1, &f.fence);
    m_batches.clear();
}

void VulkanRenderBackend::execute_commands(const std::vector<RenderDrawCmd>& cmds) {
    auto& f = m_frames[m_frame_idx];
    std::vector<float> verts;
    std::vector<uint16_t> indices;

    for (auto& cmd : cmds) {
        int x = static_cast<int>(cmd.frame.x), y = static_cast<int>(cmd.frame.y);
        int w = static_cast<int>(cmd.frame.width), h = static_cast<int>(cmd.frame.height);
        switch (cmd.type) {
            case RenderDrawCmd::RectCmd:
                tessellate_rect(verts, indices, (float)x, (float)y, (float)std::max(w, 1), (float)std::max(h, 1), cmd.color, cmd.radius_or_size);
                break;
            case RenderDrawCmd::TextCmd:
                tessellate_text(verts, indices, cmd.text, (float)x, (float)y, cmd.color, cmd.radius_or_size);
                break;
            case RenderDrawCmd::BorderCmd:
                tessellate_border(verts, indices, (float)x, (float)y, (float)std::max(w, 1), (float)std::max(h, 1), cmd.color, cmd.border_width, cmd.radius_or_size);
                break;
            case RenderDrawCmd::CircleCmd:
                tessellate_circle(verts, indices, (float)x + (float)w * 0.5f, (float)y + (float)h * 0.5f, (float)w * 0.5f, cmd.color);
                break;
            case RenderDrawCmd::LineCmd:
                tessellate_line(verts, indices, cmd.p1.x, cmd.p1.y, cmd.p2.x, cmd.p2.y, cmd.color, cmd.border_width);
                break;
        }
    }

    if (verts.empty()) return;

    VkDeviceSize vbytes = verts.size() * sizeof(float);
    VkDeviceSize ibytes = indices.size() * sizeof(uint16_t);

    if (vbytes > f.vbuf_capacity || ibytes > f.ibuf_capacity) {
        vkDestroyBuffer(m_dev, f.vbuf, nullptr);
        vkFreeMemory(m_dev, f.vbuf_mem, nullptr);
        vkDestroyBuffer(m_dev, f.ibuf, nullptr);
        vkFreeMemory(m_dev, f.ibuf_mem, nullptr);

        VkDeviceSize new_vcap = std::max(f.vbuf_capacity * 2, vbytes);
        VkDeviceSize new_icap = std::max(f.ibuf_capacity * 2, ibytes);
        create_buffer(new_vcap, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      f.vbuf, f.vbuf_mem);
        create_buffer(new_icap, VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                      f.ibuf, f.ibuf_mem);
        f.vbuf_capacity = new_vcap; f.ibuf_capacity = new_icap;
        vkMapMemory(m_dev, f.vbuf_mem, 0, new_vcap, 0, &f.vbuf_ptr);
        vkMapMemory(m_dev, f.ibuf_mem, 0, new_icap, 0, &f.ibuf_ptr);
    }

    memcpy(f.vbuf_ptr, verts.data(), (size_t)vbytes);
    memcpy(f.ibuf_ptr, indices.data(), (size_t)ibytes);

    DrawBatch batch;
    batch.index_count = (uint32_t)indices.size();
    m_batches.push_back(batch);
}

void VulkanRenderBackend::end_frame() {
    auto& f = m_frames[m_frame_idx];

    uint32_t image_idx = 0;
    VkResult res = vkAcquireNextImageKHR(m_dev, m_swapchain, UINT64_MAX, f.image_available, VK_NULL_HANDLE, &image_idx);
    if (res == VK_ERROR_OUT_OF_DATE_KHR || res == VK_SUBOPTIMAL_KHR) { recreate_swapchain(); return; }
    if (res != VK_SUCCESS) return;

    if (image_idx >= m_cmd_bufs.size() || image_idx >= m_framebuffers.size()) return;

    VkCommandBuffer cmd = m_cmd_bufs[image_idx];
    vkResetCommandBuffer(cmd, 0);
    record_commands(cmd, image_idx);

    VkSubmitInfo si{};
    si.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    VkPipelineStageFlags wait_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    si.waitSemaphoreCount = 1; si.pWaitSemaphores = &f.image_available;
    si.pWaitDstStageMask = &wait_stage;
    si.commandBufferCount = 1; si.pCommandBuffers = &cmd;
    si.signalSemaphoreCount = 1; si.pSignalSemaphores = &f.render_finished;
    vkQueueSubmit(m_graphics_queue, 1, &si, f.fence);

    VkPresentInfoKHR pi{};
    pi.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    pi.waitSemaphoreCount = 1; pi.pWaitSemaphores = &f.render_finished;
    pi.swapchainCount = 1; pi.pSwapchains = &m_swapchain;
    pi.pImageIndices = &image_idx;
    vkQueuePresentKHR(m_present_queue, &pi);

    m_frame_idx = (m_frame_idx + 1) % MAX_FRAMES;
}

void VulkanRenderBackend::present() {}

void VulkanRenderBackend::resize(int w, int h) {
    m_w = w; m_h = h;
    if (m_dev) recreate_swapchain();
}

void VulkanRenderBackend::record_commands(VkCommandBuffer cmd, uint32_t image_idx) {
    if (image_idx >= m_framebuffers.size()) return;

    VkCommandBufferBeginInfo cbbi{};
    cbbi.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(cmd, &cbbi);

    VkClearValue cv{0.12f, 0.12f, 0.15f, 1.0f};

    VkRenderPassBeginInfo rpbi{};
    rpbi.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rpbi.renderPass = m_render_pass;
    rpbi.framebuffer = m_framebuffers[image_idx];
    rpbi.renderArea = {{0,0}, m_swapchain_extent};
    rpbi.clearValueCount = 1;
    rpbi.pClearValues = &cv;

    vkCmdBeginRenderPass(cmd, &rpbi, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);

    float vp_size[2] = {(float)m_swapchain_extent.width, (float)m_swapchain_extent.height};
    vkCmdPushConstants(cmd, m_pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, 8, vp_size);

    auto& f = m_frames[m_frame_idx];
    VkDeviceSize off[1] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &f.vbuf, off);
    vkCmdBindIndexBuffer(cmd, f.ibuf, 0, VK_INDEX_TYPE_UINT16);

    for (auto& batch : m_batches) {
        vkCmdDrawIndexed(cmd, batch.index_count, 1, batch.first_index, 0, 0);
        batch.first_index += batch.index_count;
    }

    vkCmdEndRenderPass(cmd);
    vkEndCommandBuffer(cmd);
}

void VulkanRenderBackend::poll_events() {
    xcb_generic_event_t* ev;
    while ((ev = xcb_poll_for_event(m_conn))) {
        if ((ev->response_type & 0x7f) == XCB_CLIENT_MESSAGE) {
            auto* ce = (xcb_client_message_event_t*)ev;
            if (ce->data.data32[0] == m_wm_delete) m_running = false;
        }
        free(ev);
    }
    if (xcb_connection_has_error(m_conn)) m_running = false;
}

RenderBackendCapabilities VulkanRenderBackend::capabilities() const {
    RenderBackendCapabilities caps;
    caps.gpu_accelerated = true;
    caps.bindless_textures = false;
    caps.compute_shaders = false;
    caps.max_texture_size = 4096;
    caps.max_draw_calls = 65536;
    caps.renderer_name = "OVUI Vulkan";
    caps.driver_version = "1.0";
    return caps;
}

RenderStats VulkanRenderBackend::stats() const {
    RenderStats s;
    s.draw_calls = (int)m_batches.size();
    return s;
}

void register_vulkan_backend() {
    RenderBackendRegistry::instance().register_backend("vulkan", []() {
        return std::make_unique<VulkanRenderBackend>();
    });
}

} // namespace ovui
