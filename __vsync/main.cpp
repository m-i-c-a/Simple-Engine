#include <GLFW/glfw3.h>
#include <vulkan/vulkan.h>
#include <cassert>
#include <iostream>
#include <deque>
#include <chrono>
#include <array>
#include <optional>
#include <stack>
#include <unordered_map>
#include <unordered_set>
#include <thread>

#ifndef DEBUG
#define DEBUG
#endif

#include "vk_core.hpp"
#include "imgui_wrapper.hpp"
#include "Pipeline.hpp"
#include "FrameResources.hpp"

#ifdef DEBUG
#include "Stats.hpp"
#endif

#ifndef PROJECT_ROOT_DIR
#define PROJECT_ROOT_DIR ""
#endif

constexpr int32_t window_dim_x = 1200;
constexpr int32_t window_dim_y = 900;
constexpr int32_t frame_resouce_count = 3;
constexpr bool enable_blend = true;
const std::string shader_root_dir = std::string(PROJECT_ROOT_DIR) + "/__vsync/shaders/spirv/";

enum TimePoint : int
{
    StartOfFrame              = 0,
    SwapchainImageAcquireFenceWait,
    EndOfFrame                   ,
    MaxEnum
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

std::pair<VkPipeline, VkPipelineLayout> compile_program(VkFormat color_format)
{
    const VkViewport viewport {
        .x = 0.0f,
        .y = 0.0f,
        .width = window_dim_x,
        .height = window_dim_y,
        .minDepth = 0.0f,
        .maxDepth = 1.0f,
    };

    const VkRect2D scissor {
        .offset = {0, 0},
        .extent = {window_dim_x, window_dim_y}
    };

    std::vector<VkPipelineColorBlendAttachmentState> color_attachment_blend_state_vec;

    if (enable_blend)
    {
        const VkPipelineColorBlendAttachmentState blend_state {
            .blendEnable = VK_TRUE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
            .dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
            .colorBlendOp = VK_BLEND_OP_ADD,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
            .alphaBlendOp = VK_BLEND_OP_MAX,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        color_attachment_blend_state_vec.push_back(blend_state);
    }
    else
    {
        const VkPipelineColorBlendAttachmentState blend_state {
            .blendEnable = VK_FALSE,
            .srcColorBlendFactor = VK_BLEND_FACTOR_MAX_ENUM,
            .dstColorBlendFactor = VK_BLEND_FACTOR_MAX_ENUM,
            .colorBlendOp = VK_BLEND_OP_MAX_ENUM,
            .srcAlphaBlendFactor = VK_BLEND_FACTOR_MAX_ENUM,
            .dstAlphaBlendFactor = VK_BLEND_FACTOR_MAX_ENUM,
            .alphaBlendOp = VK_BLEND_OP_MAX_ENUM,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        color_attachment_blend_state_vec.push_back(blend_state);
    }

    // TODO  - have compiler check for completeness before compile call
    Compiler_GraphicsProgram compiler;
    compiler.set_shaders({shader_root_dir + "default.vert", shader_root_dir + "default.frag"});
    compiler.set_vertex_input_bindings({});
    compiler.set_vertex_input_attributes({});
    compiler.set_topology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
    compiler.set_viewport({viewport});
    compiler.set_scissor({scissor});
    compiler.set_polygon_state(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE);
    compiler.set_multisample_count(VK_SAMPLE_COUNT_1_BIT);
    compiler.set_depth_state(VK_FALSE, VK_FALSE, VK_COMPARE_OP_NEVER, {0.0f, 1.0f});
    compiler.set_color_attachment_blend_state(color_attachment_blend_state_vec);
    compiler.set_color_attachment_format({color_format});

    return compiler.compile();
}

void print_stats(const char* name, uint64_t start_tick, uint64_t end_tick, uint64_t zero_tick, float gpu_timestamp_period)
{
    const uint64_t relative_start_tick = start_tick - zero_tick;
    const uint64_t relative_end_tick = end_tick - zero_tick;

    const float start_ms = (relative_start_tick * gpu_timestamp_period) / 1000.0f;
    const float end_ms = (relative_end_tick * gpu_timestamp_period) / 1000.0f;
    const float delta_ms = ((relative_end_tick - relative_start_tick) * gpu_timestamp_period) / 1000.0f;

    std::cout << name << ": (" << start_ms << ", " << end_ms  << ") " << delta_ms << '\n';
}

std::array<float, 2> get_start_end_ms(uint64_t start_tick, uint64_t end_tick, uint64_t zero_tick, float gpu_timestamp_period)
{
    const uint64_t relative_start_tick = start_tick - zero_tick;
    const uint64_t relative_end_tick = end_tick - zero_tick;

    const float start_ms = (relative_start_tick * gpu_timestamp_period) / 1000.0f;
    const float end_ms = (relative_end_tick * gpu_timestamp_period) / 1000.0f;

    return {start_ms, end_ms};
}


void test_gui(
    const std::vector<int>& frame_id_vec, 
    const std::vector<std::vector<std::pair<std::string, std::array<uint64_t, 2>>>>& cpu_data_vec,
    const std::vector<std::vector<std::pair<std::string, std::array<uint64_t, 2>>>>& gpu_data_vec)
{
    assert(frame_id_vec.size() > 0);
    assert(frame_id_vec.size() == cpu_data_vec.size());
    assert(frame_id_vec.size() == gpu_data_vec.size());

    constexpr float y_graph_extent_padding = 1.0f;
    constexpr float y_cpu_gpu_padding = 0.25f;
    constexpr float y_frame_padding = 1.0f;

    static bool freeze = false;
    static int frame_start = 1;
    static int frame_end = 30;
    static int frame_count = frame_end;

    ImGui::Text("Freeze"); ImGui::SameLine(); 
    ImGui::Checkbox("##_freeze", &freeze); ImGui::SameLine();
    ImGui::Text("Frame Start"); ImGui::SameLine(); ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderInt("##_view_frame_start", &frame_start, 1, frame_count); ImGui::SameLine();
    ImGui::Text("Frame End"); ImGui::SameLine(); ImGui::SetNextItemWidth(100.0f);
    ImGui::SliderInt("##_view_frame_end", &frame_end, 1, frame_count);

    int orig_frame_end = frame_end;
    if (frame_end < frame_start)
        frame_end = frame_start;
    if (frame_start > orig_frame_end)
        frame_start = orig_frame_end;

    static std::vector<double> local_y_axis_tick_locations;
    static std::vector<const char*> local_frame_id_char_vec;
    static std::vector<std::vector<std::pair<std::string, std::array<float, 2>>>> local_cpu_data_vec;
    static std::vector<std::vector<std::pair<std::string, std::array<float, 2>>>> local_gpu_data_vec;
    
    static bool run_once = true;
    if (run_once)
    {
        for (int i = 0; i < frame_count; i++)
            local_y_axis_tick_locations.push_back(i);

        run_once = false;
    }

    if (!freeze || (local_frame_id_char_vec.size() != frame_count))
    {
        static std::deque<std::string> local_frame_id_str_queue;
        static std::deque<std::vector<std::pair<std::string, std::array<uint64_t, 2>>>> local_cpu_data_queue;
        static std::deque<std::vector<std::pair<std::string, std::array<uint64_t, 2>>>> local_gpu_data_queue;

        for (const auto frame_id : frame_id_vec)
            local_frame_id_str_queue.push_front(std::to_string(frame_id));

        while (local_frame_id_str_queue.size() > frame_count)
            local_frame_id_str_queue.pop_back();

        for (const auto& cpu_frame_data : cpu_data_vec)
            local_cpu_data_queue.push_back(cpu_frame_data);

        while (local_cpu_data_queue.size() > frame_count)
            local_cpu_data_queue.pop_front();

        for (const auto& gpu_frame_data : gpu_data_vec)
            local_gpu_data_queue.push_back(gpu_frame_data);

        while (local_gpu_data_queue.size() > frame_count)
            local_gpu_data_queue.pop_front();

        local_frame_id_char_vec.clear();
        for (const auto& id_str : local_frame_id_str_queue)
            local_frame_id_char_vec.push_back(id_str.c_str());

        // Calculate zero time
        if (local_cpu_data_queue.size() == frame_count)
        {
            local_cpu_data_vec.clear();
            local_gpu_data_vec.clear();

            const auto zero_tick_nano_sec = local_cpu_data_queue.front().front().second[0];

            for (const auto& cpu_frame_data : local_cpu_data_queue)
            {
                local_cpu_data_vec.push_back({});

                for (const auto& cpu_stat : cpu_frame_data)
                {
                    local_cpu_data_vec.back().push_back({});
                    local_cpu_data_vec.back().back().first = cpu_stat.first;
                    local_cpu_data_vec.back().back().second[0] = (cpu_stat.second[0] - zero_tick_nano_sec) / 1000000.0f;
                    local_cpu_data_vec.back().back().second[1] = (cpu_stat.second[1] - zero_tick_nano_sec) / 1000000.0f;
                }
            }

            for (const auto& gpu_frame_data : local_gpu_data_queue)
            {
                local_gpu_data_vec.push_back({});

                for (const auto& gpu_stat : gpu_frame_data)
                {
                    local_gpu_data_vec.back().push_back({});
                    local_gpu_data_vec.back().back().first = gpu_stat.first;
                    local_gpu_data_vec.back().back().second[0] = (gpu_stat.second[0] - zero_tick_nano_sec) / 1000000.0f;
                    local_gpu_data_vec.back().back().second[1] = (gpu_stat.second[1] - zero_tick_nano_sec) / 1000000.0f;
                }
            }
        }

#if 0
        local_cpu_data_vec.assign(local_cpu_data_queue.begin(), local_cpu_data_queue.end());
        local_gpu_data_vec.assign(local_gpu_data_queue.begin(), local_gpu_data_queue.end());

        const auto zero_time = local_cpu_data_vec.front().front().second[0];

        for (int i = 0; i < local_frame_id_char_vec.size(); i++) 
        {
            auto& cpu_frame_data = local_cpu_data_vec[i];

            for (auto& cpu_stat : cpu_frame_data)
            {
                cpu_stat.second[0] -=  zero_time;
                cpu_stat.second[1] -=  zero_time;
            }

            auto& gpu_frame_data = local_gpu_data_vec[i];

            for (auto& gpu_stat : gpu_frame_data)
            {
                gpu_stat.second[0] -=  zero_time;
                gpu_stat.second[1] -=  zero_time;
            }
        }
#endif
    }

    if (local_frame_id_char_vec.size() != frame_count)
        return;

    std::cout << "2 - Prev Frame Start      : " << local_cpu_data_vec.front().front().second[0] << '\n';
    std::cout << "2 - Prev Frame Query Start: " << local_gpu_data_vec.front().front().second[0] << '\n';
    std::cout << "2 - Prev Frame Query End  : " << local_gpu_data_vec.front().front().second[1] << '\n';
    std::cout << "2 - Prev Frame End        : " << local_cpu_data_vec.front().front().second[1] << '\n';

    ImPlot::SetNextAxesLimits(
        local_cpu_data_vec[frame_start-1].front().second[0], 
        std::max(local_cpu_data_vec[frame_end-1].back().second[1], local_gpu_data_vec[frame_end-1].back().second[1]),
        (frame_count - frame_end) - y_graph_extent_padding,
        (frame_count - frame_start) + y_graph_extent_padding,
        ImPlotCond_Always);

    if (ImPlot::BeginPlot("Frame Stats", ImVec2(-1, -1)))
    {
        ImPlot::SetupLegend(ImPlotLocation_East, ImPlotLegendFlags_Outside);
        ImPlot::SetupAxis(ImAxis_Y1, "Frame #");
        ImPlot::SetupAxisTicks(ImAxis_Y1, 
            local_y_axis_tick_locations.data() + (frame_count - frame_end),
            (frame_end - frame_start) + 1,
            local_frame_id_char_vec.data() + (frame_count - frame_end));

        float y_data[2] = {
            static_cast<float>(local_frame_id_char_vec.size() - 1),
            static_cast<float>(local_frame_id_char_vec.size() - 1)
        };

        const auto zero_time = local_cpu_data_vec[0].front().second[0];

        const std::array<ImVec4, 8> cpu_stat_colors {
            ImVec4(1, 1, 1, 1), // Frame
            ImVec4(1, 0, 0, 1), // Frame Resource Fence Wait
            ImVec4(0, 1, 0, 1), // Frame Resource Fence Wait
            ImVec4(0, 0, 1, 1), // Frame Resource Fence Wait
            ImVec4(1, 1, 0, 1), // Frame Resource Fence Wait
            ImVec4(1, 0, 1, 1), // Frame Resource Fence Wait
            ImVec4(0, 1, 1, 1), // Frame Resource Fence Wait
            ImVec4(0.25f, .5f, .75f, 1.0f), // Frame Resource Fence Wait
        };

        for (int i = 0; i < local_frame_id_char_vec.size(); i++)
        {
            int j = 0;
            for (auto& stat : local_cpu_data_vec[i])
            {
                ImPlot::SetNextLineStyle(cpu_stat_colors[j], 5);
                ImPlot::PlotLine(stat.first.c_str(), stat.second.data(), y_data, 2);
                j++;
            }

            y_data[0] -= y_cpu_gpu_padding;
            y_data[1] -= y_cpu_gpu_padding;

            for (const auto& stat : local_gpu_data_vec[i])
            {
                ImPlot::SetNextLineStyle(ImVec4(0, 1, 1, 1), 5);
                ImPlot::PlotLine(stat.first.c_str(), stat.second.data(), y_data, 2);
            }
            
            y_data[0] += y_cpu_gpu_padding;
            y_data[1] += y_cpu_gpu_padding;

            y_data[0] -= y_frame_padding;
            y_data[1] -= y_frame_padding;
        }

        // double rect_x_min_max[2] {100, 100};
        // double rect_y_min_max[2] {200, 200}; 
        // static ImPlotRect rect(0.0, 4.0, 0.0, 2.0);
        // ImPlot::DragRect(0,&rect.X.Min,&rect.Y.Min,&rect.X.Max,&rect.Y.Max,ImVec4(1,0,1,1));

        ImPlot::EndPlot();
    }
}

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *glfw_window = glfwCreateWindow(static_cast<int>(window_dim_x), static_cast<int>(window_dim_y), "App", nullptr, nullptr);
    assert(glfw_window && "Failed to create window");

    const VkPhysicalDevicePresentWaitFeaturesKHR present_wait_feature {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_WAIT_FEATURES_KHR,
        .pNext = nullptr,
        .presentWait = VK_TRUE,
    };

    const VkPhysicalDevicePresentIdFeaturesKHR present_id_feature {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PRESENT_ID_FEATURES_KHR,
        .pNext = (void*)(&present_wait_feature),
        .presentId = VK_TRUE,
    };

    const VkPhysicalDeviceVulkan13Features features_13 {
        .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES,
        .pNext = nullptr, // (void*)(&present_id_feature),
        .dynamicRendering = VK_TRUE,
    };

    const vk_core::InitInfo init_info {
        .api_version         = VK_API_VERSION_1_3,
        .instance_layers     = {
            // "VK_LAYER_KHRONOS_validation"
        },
        .instance_extensions = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            "VK_KHR_xcb_surface",
            VK_EXT_DEBUG_UTILS_EXTENSION_NAME
        },
        .glfw_window         = glfw_window,
        .physical_device_ID  = 1u,
        .queue_flags         = VK_QUEUE_GRAPHICS_BIT,
        .queue_needs_present = true, 
        .device_pnext_chain  = (void*)(&features_13),
        .device_layers       = {},
        .device_extensions   = {
            VK_KHR_SWAPCHAIN_EXTENSION_NAME, 
            VK_KHR_PRESENT_ID_EXTENSION_NAME,
            VK_KHR_PRESENT_WAIT_EXTENSION_NAME, 
            VK_NV_LOW_LATENCY_2_EXTENSION_NAME,
            VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME,
        },
        .swapchain_image_format = VK_FORMAT_B8G8R8A8_SRGB,
        .swapchain_min_image_count = 2u,
        .swapchain_image_extent = {window_dim_x , window_dim_y},
        .swapchain_present_mode = VK_PRESENT_MODE_FIFO_KHR,
        // .swapchain_present_mode = VK_PRESENT_MODE_FIFO_RELAXED_KHR,
        // .swapchain_present_mode = VK_PRESENT_MODE_IMMEDIATE_KHR,
        // .swapchain_present_mode = VK_PRESENT_MODE_MAILBOX_KHR,
    };

    vk_core::init(init_info);

    imgui_wrapper::init(glfw_window, init_info.swapchain_image_format);

    auto frame_resource_vec = std::vector<FrameResources>(frame_resouce_count);

    std::vector<Stats> frame_stats_vec(frame_resouce_count);
    std::vector<std::array<uint64_t, 2>> frame_gpu_query_data_vec(frame_resouce_count);

    const auto [vk_handle_pipeline, vk_handle_pipeline_layout] = compile_program(init_info.swapchain_image_format);

    const auto vk_handle_swapchain_image_acquire_fence = vk_core::create_fence();

    constexpr int stats_size = 1;
    std::array<VkLatencyTimingsFrameReportNV, stats_size> frame_latency_timing_array;
    bool stats_gathered = false;
    bool gather_stats = true;
    const float gpu_timestamp_period = vk_core::get_physical_device_properties().limits.timestampPeriod;

    transition_attachments_to_initial_layout();

    uint64_t frame_counter = 0lu;
    int32_t active_frame_res_idx = -1;
    uint64_t cpu_gpu_delta = 0lu;

    while (!glfwWindowShouldClose(glfw_window))
    {
        const int32_t prev_frame_res_idx = active_frame_res_idx;
        active_frame_res_idx = (active_frame_res_idx + 1) % frame_resouce_count;
        const auto& frame_resource = frame_resource_vec[active_frame_res_idx];

#ifdef DEBUG
        const auto cpu_gpu_timestamp_delta = get_calibrated_cpu_gpu_timestamp_delta();

        Stats frame_stats;
        frame_stats.push("CPU - Frame");

        const auto debug_frame_name = "Frame[" + std::to_string(frame_counter) + "][" + std::to_string(active_frame_res_idx) + "]";
        const auto debug_cmd_buff_name = "Frame[" + std::to_string(active_frame_res_idx) + "] - CommandBuffer";

        vk_core::set_latency_marker_NV(frame_counter, VK_LATENCY_MARKER_INPUT_SAMPLE_NV);
        vk_core::set_latency_marker_NV(frame_counter, VK_LATENCY_MARKER_SIMULATION_START_NV); 
        vk_core::set_latency_marker_NV(frame_counter, VK_LATENCY_MARKER_SIMULATION_END_NV);
#endif
        glfwPollEvents();

        // We must wait for the commad buffers to not be in use.
#ifdef DEBUG
        frame_stats.push("CPU - Fence - Frame Resource");
#endif

        vk_core::wait_for_fence(frame_resource.vk_handle_fence, UINT64_MAX);
        vk_core::reset_fence(frame_resource.vk_handle_fence);

#ifdef DEBUG
        frame_stats.pop();

        std::array<uint64_t, 2> frame_gpu_query_data;

        frame_stats.push("CPU - Query Pool Results Call");
        if (frame_counter > frame_resouce_count)
            vk_core::get_query_pool_results(frame_resource.vk_handle_query_pool, 0, 2, sizeof(uint64_t) * 2, frame_gpu_query_data.data(), sizeof(uint64_t), VK_QUERY_RESULT_64_BIT | VK_QUERY_RESULT_WAIT_BIT);
        frame_stats.pop();

        frame_stats.push("CPU - Image Acquire Call");
#endif

        // Acquire index of next presentable image in the swapchain. This function blocks until an image can be acquired.
        // The presentation engine may not be done using the image on return.
        const auto next_avail_swapchain_image_idx = vk_core::acquire_next_swapchain_image(VK_NULL_HANDLE, vk_handle_swapchain_image_acquire_fence);

        vk_core::reset_command_pool(frame_resource.vk_handle_cmd_pool);
        vk_core::begin_command_buffer(frame_resource.vk_handle_cmd_buff, VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
    
#ifdef DEBUG
        frame_stats.pop();
        vk_core::debug_utils_begin_label(frame_resource.vk_handle_cmd_buff, debug_cmd_buff_name.c_str());
        frame_stats.push("CPU - Record");
#endif

        // Command Buffer Record
        {
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

            const VkClearValue clear_value {
                .color = {0.1f, 0.1f, 0.1f, 0.0f}
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
                .clearValue = clear_value
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

#ifdef DEBUG 
            vkCmdResetQueryPool(frame_resource.vk_handle_cmd_buff, frame_resource.vk_handle_query_pool, 0, 2);
            vkCmdWriteTimestamp(frame_resource.vk_handle_cmd_buff, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, frame_resource.vk_handle_query_pool, 0);
#endif

            vkCmdPipelineBarrier(frame_resource.vk_handle_cmd_buff,
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0u, nullptr,
                0u, nullptr,
                1u, &transition_to_render_image_barrier);

            vkCmdBeginRendering(frame_resource.vk_handle_cmd_buff, &rendering_info);

#ifdef DEBUG
            vk_core::debug_utils_begin_label(frame_resource.vk_handle_cmd_buff, "render");
#endif

            {
                vkCmdBindPipeline(frame_resource.vk_handle_cmd_buff, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_handle_pipeline);
                vkCmdDraw(frame_resource.vk_handle_cmd_buff, 3, 10000, 0, 0);
                std::this_thread::sleep_for(std::chrono::milliseconds(18));
            }

#ifdef DEBUG
            if (frame_counter > (frame_resouce_count * 2))
            {
                ImGui_ImplVulkan_NewFrame();
                ImGui_ImplGlfw_NewFrame();
                ImGui::NewFrame();

                static int max_size = 1;

                static std::vector<int> frame_id_vec;
                static std::vector<std::vector<std::pair<std::string, std::array<uint64_t, 2>>>> cpu_data_vec;
                static std::vector<std::vector<std::pair<std::string, std::array<uint64_t, 2>>>> gpu_data_vec;

                frame_id_vec.push_back(frame_counter - 1);
                cpu_data_vec.push_back(frame_stats_vec[active_frame_res_idx].get_cpu_data()); // 2 frames ago
                gpu_data_vec.push_back({});
                gpu_data_vec.back().push_back({});
                gpu_data_vec.back().front().first = "GPU";
                gpu_data_vec.back().front().second = {
                    frame_gpu_query_data[0] - cpu_gpu_timestamp_delta,
                    frame_gpu_query_data[1] - cpu_gpu_timestamp_delta,
                };

                if (frame_id_vec.size() > max_size)
                {
                    frame_id_vec.erase(frame_id_vec.begin());
                    cpu_data_vec.erase(cpu_data_vec.begin());
                    gpu_data_vec.erase(gpu_data_vec.begin());
                }

                // I do not have my previous frame's GPU queries at this point. The GPU work for the previous frame
                // could still be executing.
                // I do have GPU work for (frame_counter-1) (this frame resources' previous run).

                std::cout << "\n0 - Prev Frame Start      : " << cpu_data_vec.front().front().second[0] << '\n';
                std::cout <<   "0 - Prev Frame Query Start: " << frame_gpu_query_data[0] - cpu_gpu_timestamp_delta << '\n';
                std::cout <<   "0 - Prev Frame Query End  : " << frame_gpu_query_data[1] - cpu_gpu_timestamp_delta << '\n';
                std::cout <<   "0 - Prev Frame End        : " << cpu_data_vec.front().front().second[1] << '\n';

                const uint64_t divider = 1000;
                const uint64_t zero_tick = cpu_data_vec.front().front().second[0];

                std::cout << "1 - Prev Frame Start      : " << ((cpu_data_vec.front().front().second[0]) - zero_tick)  / divider << '\n';
                std::cout << "1 - Prev Frame Query Start: " << (((gpu_data_vec.back().front().second[0])     ) - zero_tick)  / divider << '\n';
                std::cout << "1 - Prev Frame Query End  : " << (((gpu_data_vec.back().front().second[1])     ) - zero_tick)  / divider << '\n';
                std::cout << "1 - Prev Frame End        : " << ((cpu_data_vec.front().front().second[1]) - zero_tick)  / divider << '\n';

                test_gui(frame_id_vec, cpu_data_vec, gpu_data_vec);

                // ImPlot::ShowDemoWindow();

                ImGui::Render();
                ImDrawData* draw_data = ImGui::GetDrawData();
                ImGui_ImplVulkan_RenderDrawData(draw_data, frame_resource.vk_handle_cmd_buff);
            }

            vk_core::debug_utils_end_label(frame_resource.vk_handle_cmd_buff);
#endif

            vkCmdEndRendering(frame_resource.vk_handle_cmd_buff);

            vkCmdPipelineBarrier(frame_resource.vk_handle_cmd_buff, 
                VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
                VK_DEPENDENCY_BY_REGION_BIT,
                0u, nullptr,
                0u, nullptr,
                1u, &transition_to_present_image_barrier);

#ifdef DEBUG
            vkCmdWriteTimestamp(frame_resource.vk_handle_cmd_buff, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, frame_resource.vk_handle_query_pool, 1);
#endif
        }

#ifdef DEBUG
        frame_stats.pop();
        vk_core::debug_utils_end_label(frame_resource.vk_handle_cmd_buff);
#endif

        vk_core::end_command_buffer(frame_resource.vk_handle_cmd_buff);

        // Wait for the prpesentation engine to be finished with the image.
#ifdef DEBUG
        frame_stats.push("CPU - Fence - Image Acquire");
#endif

        vk_core::wait_for_fence(vk_handle_swapchain_image_acquire_fence, UINT64_MAX);
        vk_core::reset_fence(vk_handle_swapchain_image_acquire_fence);

#ifdef DEBUG
        frame_stats.pop();
        frame_stats.push("CPU - Submit");
        vk_core::set_latency_marker_NV(frame_counter, VK_LATENCY_MARKER_RENDERSUBMIT_START_NV);
#endif

        // Submit
        {
            const VkPipelineStageFlags render_submit_wait_sem4_stages = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

            const VkLatencySubmissionPresentIdNV latency_submission_present {
                .sType = VK_STRUCTURE_TYPE_LATENCY_SUBMISSION_PRESENT_ID_NV,
                .pNext = nullptr,
                .presentID = frame_counter,
            };

            const VkSubmitInfo submit_info {
                .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
                .pNext = &latency_submission_present,
                .waitSemaphoreCount = 0u,
                .pWaitSemaphores = nullptr,
                .pWaitDstStageMask = 0u,
                .commandBufferCount = 1u,
                .pCommandBuffers = &frame_resource.vk_handle_cmd_buff,
                .signalSemaphoreCount = 1u,
                .pSignalSemaphores = &frame_resource.vk_handle_render_complete_sem4,
            };

            vk_core::queue_submit(submit_info, frame_resource.vk_handle_fence);
            // vk_core::device_wait_idle();
        }

#ifdef DEBUG
    frame_stats.pop();
    frame_stats.push("CPU - Present Call");
    vk_core::set_latency_marker_NV(frame_counter, VK_LATENCY_MARKER_RENDERSUBMIT_END_NV);
    vk_core::set_latency_marker_NV(frame_counter, VK_LATENCY_MARKER_PRESENT_START_NV);
#endif

        // Present
        {
            const VkPresentIdKHR present_id_obj {
                .sType = VK_STRUCTURE_TYPE_PRESENT_ID_KHR,
                .pNext = nullptr,
                .swapchainCount = 1u,
                .pPresentIds = &frame_counter 
            }; 

            vk_core::present(next_avail_swapchain_image_idx, {frame_resource.vk_handle_render_complete_sem4}, (void*)(&present_id_obj));
        }

#ifdef DEBUG
    frame_stats.pop();
    vk_core::set_latency_marker_NV(frame_counter, VK_LATENCY_MARKER_PRESENT_END_NV);
#endif

#if 0
        {
            for (uint32_t i = 0; i < frame_latency_timing_array.size(); i++)
            {
                frame_latency_timing_array[i].sType = VK_STRUCTURE_TYPE_LATENCY_TIMINGS_FRAME_REPORT_NV;
                frame_latency_timing_array[i].pNext = nullptr;
                frame_latency_timing_array[i].presentID = 0u;
            }

            VkGetLatencyMarkerInfoNV latency_marker_info {
                .sType = VK_STRUCTURE_TYPE_GET_LATENCY_MARKER_INFO_NV,
                .pNext = nullptr,
                .timingCount = static_cast<uint32_t>(frame_latency_timing_array.size()),
                .pTimings = frame_latency_timing_array.data(),
            };

            vk_core::get_latency_timings_NV(&latency_marker_info);

            const uint64_t zero_tick = frame_latency_timing_array.front().inputSampleTimeUs;

            for (const auto& frame_latency_timings : frame_latency_timing_array)
            {
                std::cout << frame_counter << " " << frame_latency_timings.presentID << '\n';
                print_stats("\tSimTime     ", frame_latency_timings.simStartTimeUs, frame_latency_timings.simEndTimeUs, zero_tick, gpu_timestamp_period);
                print_stats("\tRenderSubmit", frame_latency_timings.renderSubmitStartTimeUs, frame_latency_timings.renderSubmitEndTimeUs, zero_tick, gpu_timestamp_period);
                print_stats("\tGPURender   ", frame_latency_timings.gpuRenderStartTimeUs, frame_latency_timings.gpuRenderEndTimeUs, zero_tick, gpu_timestamp_period);
                print_stats("\tPresent     ", frame_latency_timings.presentStartTimeUs, frame_latency_timings.presentEndTimeUs, zero_tick, gpu_timestamp_period);
                // print_stats("Driver      ", frame_latency_timings.driverStartTimeUs, frame_latency_timings.driverEndTimeUs, zero_tick, gpu_timestamp_period);
                print_stats("\tOsRenderQ   ", frame_latency_timings.osRenderQueueStartTimeUs, frame_latency_timings.osRenderQueueEndTimeUs, zero_tick, gpu_timestamp_period);
                std::cout << '\n';
            }
        }
#endif

#ifdef DEBUG
        frame_stats.pop();
        frame_stats_vec[active_frame_res_idx] = frame_stats;
        frame_gpu_query_data_vec[active_frame_res_idx] = frame_gpu_query_data;
#endif
        frame_counter++;
    }

    vk_core::queue_wait_idle();

    for (auto& frame_resource : frame_resource_vec)
    {
        vk_core::destroy_command_pool(frame_resource.vk_handle_cmd_pool);
        vk_core::destroy_fence(frame_resource.vk_handle_fence);
        vk_core::destroy_semaphore(frame_resource.vk_handle_render_complete_sem4);
        vk_core::destroy_semaphore(frame_resource.vk_handle_swapchain_image_acquire_sem4);
    }

    vk_core::destroy_pipeline_layout(vk_handle_pipeline_layout);
    vk_core::destroy_pipeline(vk_handle_pipeline);
    vk_core::destroy_fence(vk_handle_swapchain_image_acquire_fence);
    
    imgui_wrapper::destroy();

    vk_core::terminate();

    glfwDestroyWindow(glfw_window);
    glfwTerminate();

    return 0;
}