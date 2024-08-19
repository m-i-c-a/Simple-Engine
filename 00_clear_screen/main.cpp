#include "vk_core.hpp"

#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cassert>
#include <iostream>

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR ""
#endif

constexpr int32_t window_dim_x = 500;
constexpr int32_t window_dim_y = 500;
constexpr int32_t frame_resouce_count = 1;
const std::string vulkan_state_path = std::string(PROJECT_ROOT_DIR) + "/00_clear_screen/vulkan_state.json";

struct FrameResources
{
    VkCommandPool vk_handle_cmd_pool = VK_NULL_HANDLE;
    VkCommandBuffer vk_handle_cmd_buff = VK_NULL_HANDLE;
    VkFence vk_handle_fence = VK_NULL_HANDLE;
    VkSemaphore vk_handle_sem4 = VK_NULL_HANDLE;
};

void transition_attachments_to_initial_layout()
{
    const auto vk_handle_cmd_pool = vk_core::create_command_pool();
    const auto vk_handle_cmd_buff = vk_core::allocate_command_buffer(vk_handle_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    const VkCommandBufferBeginInfo cmd_buff_begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .pNext = nullptr,
        .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        .pInheritanceInfo = nullptr,
    };

    const VkImageMemoryBarrier transition_to_present_barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
        .srcQueueFamilyIndex = vk_core::get_queue_family_idx(),
        .dstQueueFamilyIndex = vk_core::get_queue_family_idx(),
        .image = VK_NULL_HANDLE,
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0u,
            .levelCount = 1u,
            .baseArrayLayer = 0u,
            .layerCount = 1u,
        },
    }; 

    std::vector<VkImageMemoryBarrier> swapchain_transition_to_present_barrier_vec(vk_core::get_swapchain_image_count(), transition_to_present_barrier);

    for (auto i = 0; i < swapchain_transition_to_present_barrier_vec.size(); i++)
        swapchain_transition_to_present_barrier_vec[i].image = vk_core::get_swapchain_image(i);
    
    const VkSubmitInfo submit_info {
        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
        .pNext = nullptr,
        .waitSemaphoreCount = 0u,
        .pWaitSemaphores = nullptr,
        .pWaitDstStageMask = 0u,
        .commandBufferCount = 1u,
        .pCommandBuffers = &vk_handle_cmd_buff,
        .signalSemaphoreCount = 0u,
        .pSignalSemaphores = nullptr,
    };

    vkBeginCommandBuffer(vk_handle_cmd_buff, &cmd_buff_begin_info);

    vkCmdPipelineBarrier(vk_handle_cmd_buff, 
        VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
        VK_DEPENDENCY_BY_REGION_BIT,
        0u, nullptr,
        0u, nullptr,
        static_cast<uint32_t>(swapchain_transition_to_present_barrier_vec.size()), swapchain_transition_to_present_barrier_vec.data());

    vkEndCommandBuffer(vk_handle_cmd_buff);

    vk_core::queue_submit(submit_info, VK_NULL_HANDLE);
    vk_core::queue_wait_idle();

    vk_core::destroy_command_pool(vk_handle_cmd_pool);
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *glfw_window = glfwCreateWindow(static_cast<int>(window_dim_x), static_cast<int>(window_dim_y), "App", nullptr, nullptr);
    assert(glfw_window && "Failed to create window");

    const VkPhysicalDeviceVulkan13Features features_13 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .dynamicRendering = VK_TRUE
    };

    vk_core::InitInfo init_info {
        .api_version         = VK_API_VERSION_1_3,
        .instance_layers     = {"VK_LAYER_KHRONOS_validation"},
        .instance_extensions = {"VK_KHR_surface", "VK_KHR_xcb_surface"},
        .glfw_window         = glfw_window,
        .physical_device_ID  = 1u,
        .queue_flags         = VK_QUEUE_GRAPHICS_BIT,
        .queue_needs_present = true, 
        .device_pnext_chain  = (void*)(&features_13), 
        .device_layers       = {},
        .device_extensions   = {"VK_KHR_swapchain"},
        .swapchain_image_format = VK_FORMAT_B8G8R8A8_SRGB,
        .swapchain_min_image_count = 2u,
        .swapchain_image_extent = {window_dim_x , window_dim_y},
        .swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR,
    };

    vk_core::init(init_info);

    transition_attachments_to_initial_layout();

    const auto vk_handle_swapchain_image_acquire_fence = vk_core::create_fence();

    std::vector<FrameResources> frame_resource_vec(frame_resouce_count);

    for (auto& frame_resource : frame_resource_vec)
    {
        frame_resource.vk_handle_cmd_pool = vk_core::create_command_pool(VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
        frame_resource.vk_handle_cmd_buff = vk_core::allocate_command_buffer(frame_resource.vk_handle_cmd_pool, VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        frame_resource.vk_handle_fence = vk_core::create_fence(VK_FENCE_CREATE_SIGNALED_BIT);
        frame_resource.vk_handle_sem4 = vk_core::create_semaphore();
    }

    VkGraphicsPipelineCreateInfo create_info {
        // .sType = ,
        // .pNext = ,
        // .flags = ,
        // .stageCount = ,
        // .pStages = ,
        // .pVertexInputState = ,
        // .pInputAssemblyState = ,
        // .pTessellationState = ,
        // .pViewportState = ,
        // .pRasterizationState = ,
        // .pMultisampleState = ,
        // .pDepthStencilState = ,
        // .pColorBlendState = ,
        // .pDynamicState = ,
        // .layout = ,
        // .renderPass = ,
        // .subpass = ,
        // .basePipelineHandle = ,
        // .basePipelineIndex = ,
    };

    int32_t active_frame_res_idx = -1;

    while (!glfwWindowShouldClose(glfw_window))
    {
        glfwPollEvents();

        active_frame_res_idx = (active_frame_res_idx + 1) % frame_resouce_count;
        const auto& frame_resource = frame_resource_vec[active_frame_res_idx];

        // Acquire index of next presentable image in the swapchain. This function blocks until an image can be acquired.
        // The presentation engine may not be done using the image on return.
        const auto next_avail_swapchain_image_idx = vk_core::acquire_next_swapchain_image(VK_NULL_HANDLE, vk_handle_swapchain_image_acquire_fence);

        // Wait for the prpesentation engine to be finished with the image.
        vk_core::wait_for_fence(vk_handle_swapchain_image_acquire_fence, UINT64_MAX);
        vk_core::reset_fence(vk_handle_swapchain_image_acquire_fence);

        // We being using frame resources by reseting the active frame resource's command pool.
        // We must wait for the commad buffers to not be in use.
        vk_core::wait_for_fence(frame_resource.vk_handle_fence, UINT64_MAX);
        vk_core::reset_fence(frame_resource.vk_handle_fence);

        vk_core::reset_command_pool(frame_resource.vk_handle_cmd_pool);

        const VkImageMemoryBarrier transition_to_render_image_barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .srcQueueFamilyIndex = vk_core::get_queue_family_idx(),
            .dstQueueFamilyIndex = vk_core::get_queue_family_idx(),
            .image = vk_core::get_swapchain_image(next_avail_swapchain_image_idx),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u,
            },
        }; 

        const VkImageMemoryBarrier transition_to_present_image_barrier {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .pNext = nullptr,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = vk_core::get_queue_family_idx(),
            .dstQueueFamilyIndex = vk_core::get_queue_family_idx(),
            .image = vk_core::get_swapchain_image(next_avail_swapchain_image_idx),
            .subresourceRange = {
                .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                .baseMipLevel = 0u,
                .levelCount = 1u,
                .baseArrayLayer = 0u,
                .layerCount = 1u,
            },
        }; 

        const VkCommandBufferBeginInfo cmd_buff_begin_info {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .pNext = nullptr,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
            .pInheritanceInfo = nullptr,
        };

        const VkRenderingAttachmentInfo color_attachment_info {
            .sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO,
            .pNext = nullptr,
            .imageView = vk_core::get_swapchain_image_view(next_avail_swapchain_image_idx),
            .imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .resolveMode = VK_RESOLVE_MODE_NONE,
            .resolveImageView = VK_NULL_HANDLE,
            .resolveImageLayout = VK_IMAGE_LAYOUT_UNDEFINED,
            .loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR,
            .storeOp = VK_ATTACHMENT_STORE_OP_STORE,
            .clearValue = {.color={0.0f, 0.63f, 0.11f, 0.0f}}
        };

        const VkRenderingInfo rendering_info {
            .sType = VK_STRUCTURE_TYPE_RENDERING_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .renderArea = {.offset={}, .extent={window_dim_x, window_dim_y}},
            .layerCount = 1u,
            .viewMask = 0x0,
            .colorAttachmentCount = 1u,
            .pColorAttachments = &color_attachment_info,
            .pDepthAttachment = nullptr,
            .pStencilAttachment = nullptr,
        };

        const VkSubmitInfo submit_info {
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .pNext = nullptr,
            .waitSemaphoreCount = 0u,
            .pWaitSemaphores = nullptr,
            .pWaitDstStageMask = 0u,
            .commandBufferCount = 1u,
            .pCommandBuffers = &frame_resource.vk_handle_cmd_buff,
            .signalSemaphoreCount = 1u,
            .pSignalSemaphores = &frame_resource.vk_handle_sem4,
        };

        vkBeginCommandBuffer(frame_resource.vk_handle_cmd_buff, &cmd_buff_begin_info);

        vkCmdPipelineBarrier(frame_resource.vk_handle_cmd_buff, 
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0u, nullptr,
            0u, nullptr,
            1u, &transition_to_render_image_barrier);

        vkCmdBeginRendering(frame_resource.vk_handle_cmd_buff, &rendering_info);

        {

        }

        vkCmdEndRendering(frame_resource.vk_handle_cmd_buff);

        vkCmdPipelineBarrier(frame_resource.vk_handle_cmd_buff, 
            VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
            VK_DEPENDENCY_BY_REGION_BIT,
            0u, nullptr,
            0u, nullptr,
            1u, &transition_to_present_image_barrier);

        vkEndCommandBuffer(frame_resource.vk_handle_cmd_buff);

        vk_core::queue_submit(submit_info, frame_resource.vk_handle_fence);
        vk_core::present(next_avail_swapchain_image_idx, {frame_resource.vk_handle_sem4});
    }

    vk_core::queue_wait_idle();

    for (auto& frame_resource : frame_resource_vec)
    {
        vk_core::destroy_command_pool(frame_resource.vk_handle_cmd_pool);
        vk_core::destroy_fence(frame_resource.vk_handle_fence);
        vk_core::destroy_semaphore(frame_resource.vk_handle_sem4);
    }

    vk_core::destroy_fence(vk_handle_swapchain_image_acquire_fence);
    
    vk_core::terminate();

    glfwDestroyWindow(glfw_window);
    glfwTerminate();

    return 0;
}