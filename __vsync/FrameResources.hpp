#ifndef FRAME_RESOURCES_HPP
#define FRAME_RESOURCES_HPP

#include <vulkan/vulkan.h>
#include <memory>

struct FrameResources
{
public:
    FrameResources();

    const VkCommandPool   vk_handle_cmd_pool {VK_NULL_HANDLE};
    const VkCommandBuffer vk_handle_cmd_buff {VK_NULL_HANDLE};
    const VkFence         vk_handle_fence {VK_NULL_HANDLE};
    const VkSemaphore     vk_handle_swapchain_image_acquire_sem4 {VK_NULL_HANDLE};
    const VkSemaphore     vk_handle_render_complete_sem4 {VK_NULL_HANDLE};

#ifdef DEBUG
    const VkQueryPool     vk_handle_query_pool {VK_NULL_HANDLE};
#endif
};

#endif