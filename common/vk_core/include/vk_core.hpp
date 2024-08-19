#ifndef VK_CORE_HPP
#define VK_CORE_HPP

#include <vulkan/vulkan.h>

#include <string_view>
#include <vector>
#include <optional>

class GLFWwindow;

namespace vk_core
{
    VkCommandPool create_command_pool(VkCommandPoolCreateFlags flags = 0x0);
    VkCommandPool create_command_pool(const char* name, VkCommandPoolCreateFlags flags = 0x0);
    VkSemaphore create_semaphore(VkSemaphoreCreateFlags flags = 0x0);
    VkSemaphore create_semaphore(const char* name, VkSemaphoreCreateFlags flags = 0x0);
    VkFence create_fence(VkFenceCreateFlags flags = 0x0);
    VkFence create_fence(const char* name, VkFenceCreateFlags flags = 0x0);

    VkQueryPool create_query_pool(VkQueryType type, uint32_t count, VkQueryPipelineStatisticFlags pipeline_stat_flags = 0x0, VkQueryPoolCreateFlags query_pool_flags = 0x0, void* p_next = nullptr);

    void get_query_pool_results(VkQueryPool vk_handle_query_pool, uint32_t first_query, uint32_t query_count, size_t data_size, void* data, VkDeviceSize stride, VkQueryResultFlags flags);
    void get_calibrated_timestamps(const VkCalibratedTimestampInfoKHR* timestamp_infos, uint64_t* timestamps, uint32_t count, uint64_t* max_deviation);

    // Startup / Cleanup

    struct InitInfo
    {
        uint32_t api_version;
        std::vector<const char*> instance_layers;
        std::vector<const char*> instance_extensions;
        GLFWwindow* glfw_window;
        uint32_t physical_device_ID;
        VkQueueFlags queue_flags;
        bool queue_needs_present;
        void* device_pnext_chain;
        std::vector<const char*> device_layers;
        std::vector<const char*> device_extensions;
        VkFormat swapchain_image_format;
        uint32_t swapchain_min_image_count;
        VkExtent2D swapchain_image_extent;
        VkPresentModeKHR swapchain_present_mode;
    };

    void init(const InitInfo& init_info);
    void terminate();

    VkInstance get_instance();
    VkPhysicalDevice get_physical_device();
    VkDevice get_device();
    VkQueue get_queue();
    uint32_t get_queue_family_idx();
    int32_t get_swapchain_image_count();
    VkImage get_swapchain_image(int32_t idx);





    void present(uint32_t swapchain_image_idx, std::vector<VkSemaphore>&& vk_handle_wait_sem4_vec, void* p_next = nullptr);
    void wait_for_present(uint64_t present_id, uint64_t timeout);

    void device_wait_idle();

    void queue_submit(const VkSubmitInfo& submit_info, VkFence vk_handle_signal_fence = VK_NULL_HANDLE);
    void queue_wait_idle();

    void destroy_semaphore(VkSemaphore vk_handle_sem4);

    void wait_for_fence(VkFence vk_handle_fence, uint64_t timeout);
    void reset_fence(VkFence vk_handle_fence);

    void reset_command_pool(VkCommandPool vk_handle_cmd_pool);
    void destroy_command_pool(VkCommandPool vk_handle_cmd_pool);

    VkCommandBuffer allocate_command_buffer(const char* name, VkCommandPool cmd_pool, VkCommandBufferLevel level);
    VkCommandBuffer allocate_command_buffer(VkCommandPool cmd_pool, VkCommandBufferLevel level);
    void begin_command_buffer(VkCommandBuffer vk_handle_cmd_buff, VkCommandBufferUsageFlags flags);
    void end_command_buffer(VkCommandBuffer vk_handle_cmd_buff);

    VkDescriptorPool create_desc_pool(const VkDescriptorPoolCreateInfo& create_info);
    void destroy_desc_pool(VkDescriptorPool vk_handle_desc_pool);

    VkPipelineLayout create_pipeline_layout(const VkPipelineLayoutCreateInfo& create_info);
    void destroy_pipeline_layout(VkPipelineLayout vk_handle_pipeline_layout);

    
    VkImageView get_swapchain_image_view(uint32_t idx);
    uint32_t acquire_next_swapchain_image(VkSemaphore vk_handle_signal_sem4, VkFence vk_handle_signal_fence);

    VkImageMemoryBarrier get_active_swapchain_image_memory_barrier(const VkAccessFlags src_access_flags, const VkAccessFlags dst_access_flags, const VkImageLayout old_layout, const VkImageLayout new_layout);
    VkImage get_active_swapchain_image();

    void wait_for_fences(uint32_t fence_count, VkFence* vk_handle_fence_list, VkBool32 wait_all, uint64_t timeout);
    void reset_fences(const uint32_t fence_count, const VkFence* vk_handle_fence_list);
    void destroy_fence(const VkFence vk_handle_fence);

    const VkPhysicalDeviceProperties& get_physical_device_properties();

    void debug_utils_begin_label(VkCommandBuffer vk_handle_cmd_buff, const char* name);
    void debug_utils_end_label(VkCommandBuffer vk_handle_cmd_buff);

    void get_latency_timings_NV(VkGetLatencyMarkerInfoNV* latency_marker_info);
    void set_latency_marker_NV(uint64_t present_id, VkLatencyMarkerNV marker);

    // Resource Management

    VkSampler create_sampler(const VkSamplerCreateInfo& create_info);

    VkImage create_image(const VkImageCreateInfo& create_info);
    VkImageView create_image_view(const VkImageViewCreateInfo& create_info);
    VkDeviceMemory allocate_image_memory(const VkImage vk_handle_image, const VkMemoryPropertyFlags flags);
    void bind_image_memory(const VkImage vk_handle_image, const VkDeviceMemory vk_handle_image_memory);
    void destroy_image(const VkImage vk_handle_image);
    void destroy_image_view(const VkImageView vk_handle_image_view);

    VkBuffer create_buffer(const VkBufferCreateInfo& create_info);
    VkDeviceMemory allocate_buffer_memory(const VkBuffer vk_handle_buffer, const VkMemoryPropertyFlags flags, VkDeviceSize& size);
    void bind_buffer_memory(const VkBuffer vk_handle_buffer, const VkDeviceMemory vk_handle_buffer_memory); 
    void destroy_buffer(const VkBuffer vk_handle_buffer);

    void map_memory(const VkDeviceMemory memory, const VkDeviceSize offset, const VkDeviceSize size, const VkMemoryMapFlags flags, void** data);
    void unmap_memory(const VkDeviceMemory vk_handle_memory);
    void free_memory(const VkDeviceMemory vk_handle_memory);

    VkShaderModule create_shader_module(const VkShaderModuleCreateInfo& create_info);
    void destroy_shader_module(const VkShaderModule vk_handle_shader_module);

    VkPipeline create_graphics_pipeline(const VkGraphicsPipelineCreateInfo& create_info);
    void destroy_pipeline(const VkPipeline vk_handle_pipeline);



    VkDescriptorSetLayout create_desc_set_layout(const VkDescriptorSetLayoutCreateInfo& create_info);
    void destroy_desc_set_layout(const VkDescriptorSetLayout vk_handle_desc_set_layout);

    std::vector<VkDescriptorSet> allocate_desc_sets(const VkDescriptorSetAllocateInfo& alloc_info);
    void update_desc_sets(const uint32_t update_count, const VkWriteDescriptorSet* const p_write_desc_set_list, const uint32_t copy_count, const VkCopyDescriptorSet* const p_copy_desc_set_list);


    // Events


    // Misc

};


#endif // VK_CORE_HPP