#include "FrameResources.hpp"
#include "vk_core.hpp"

FrameResources::FrameResources()
    : vk_handle_cmd_pool(vk_core::create_command_pool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT))
    , vk_handle_cmd_buff(vk_core::allocate_command_buffer(vk_handle_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY))
    , vk_handle_fence(vk_core::create_fence(VK_FENCE_CREATE_SIGNALED_BIT))
    , vk_handle_swapchain_image_acquire_sem4(vk_core::create_semaphore())
    , vk_handle_render_complete_sem4(vk_core::create_semaphore())
#ifdef DEBUG
    , vk_handle_query_pool(vk_core::create_query_pool(VK_QUERY_TYPE_TIMESTAMP, 2))
#endif
{
}