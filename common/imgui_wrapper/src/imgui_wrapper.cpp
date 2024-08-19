#include "imgui_wrapper.hpp"

#include "vk_core.hpp"
#include <array>

namespace
{
    VkDescriptorPool vk_handle_desc_pool = VK_NULL_HANDLE;

    void create_desc_pool()
    {
        // No idea how many descriptors imgui needs
        constexpr std::array<VkDescriptorPoolSize, 11> pool_sizes {{
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        }};

        const VkDescriptorPoolCreateInfo pool_create_info {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .pNext = nullptr,
            .flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT,
            .maxSets = 2u,
            .poolSizeCount = static_cast<uint32_t>(pool_sizes.size()),
            .pPoolSizes = pool_sizes.data(),
        };

        vk_handle_desc_pool = vk_core::create_desc_pool(pool_create_info);
    }
};

namespace imgui_wrapper
{
    void init(GLFWwindow* glfw_window, VkFormat color_attachment_format)
    {
        create_desc_pool();

        // Setup Dear ImGui context
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImPlot::CreateContext();
        ImGuiIO& io = ImGui::GetIO(); (void)io;

        // Setup Dear ImGui style
        ImGui::StyleColorsDark();

        // Setup Platform/Renderer backends
        ImGui_ImplGlfw_InitForVulkan(glfw_window, true);
        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = vk_core::get_instance();
        init_info.PhysicalDevice = vk_core::get_physical_device();
        init_info.Device = vk_core::get_device();
        init_info.QueueFamily = vk_core::get_queue_family_idx();
        init_info.Queue = vk_core::get_queue();
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = vk_handle_desc_pool;
        init_info.Subpass = 0;
        init_info.UseDynamicRendering = true;
        init_info.PipelineRenderingCreateInfo = {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR,
            .pNext = nullptr,
            .viewMask = 0x0,
            .colorAttachmentCount = 1u,
            .pColorAttachmentFormats = &color_attachment_format,
            .depthAttachmentFormat = VK_FORMAT_UNDEFINED,
            .stencilAttachmentFormat = VK_FORMAT_UNDEFINED };
        init_info.MinImageCount = 2;
        init_info.ImageCount = static_cast<uint32_t>(vk_core::get_swapchain_image_count());
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.CheckVkResultFn = nullptr;
        ImGui_ImplVulkan_Init(&init_info);
        ImGui_ImplVulkan_CreateFontsTexture();
    }


    void destroy()
    {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImPlot::DestroyContext();
        ImGui::DestroyContext();

        vk_core::destroy_desc_pool(vk_handle_desc_pool);
    }
}