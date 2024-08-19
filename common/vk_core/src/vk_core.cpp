#include "vk_core.hpp"

#include <vulkan/vulkan.h>

#include <GLFW/glfw3.h>
#include "json.hpp"

#include <stdio.h>
#include <assert.h>
#include <fstream>
#include <vector>
#include <algorithm>

#define LOG(fmt, ...)                    \
    fprintf(stdout, fmt, ##__VA_ARGS__); \
    fflush(stdout);

#define ASSERT(val, fmt, ...)                    \
    do                                           \
    {                                            \
        if (!(val))                                \
        {                                        \
            fprintf(stdout, fmt, ##__VA_ARGS__); \
            fflush(stdout);                      \
            assert(false);                       \
        }                                        \
    } while(0)                                   \

#define EXIT(fmt, ...)                       \
    do                                       \
    {                                        \
        fprintf(stderr, fmt, ##__VA_ARGS__); \
        fflush(stderr);                      \
        assert(false);                       \
    } while (0)

#define VK_CHECK(val)                  \
    do                                 \
    {                                  \
        if (val != VK_SUCCESS)         \
        {                              \
            assert(val == VK_SUCCESS); \
        }                              \
    } while (false)

static VkInstance create_instance(uint32_t api_version, const std::vector<const char*>& layer_vec, const std::vector<const char*>& extension_vec) 
{
    const VkApplicationInfo app_info {
        .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
        .pApplicationName = "",
        .applicationVersion = 0u,
        .pEngineName = "",
        .engineVersion = 0u,
        .apiVersion = api_version
    };

    const VkInstanceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
        .pApplicationInfo = &app_info,
        .enabledLayerCount = static_cast<uint32_t>(layer_vec.size()),
        .ppEnabledLayerNames = layer_vec.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extension_vec.size()),
        .ppEnabledExtensionNames = extension_vec.data(),
    };

    VkInstance instance = VK_NULL_HANDLE;
    VK_CHECK(vkCreateInstance(&create_info, nullptr, &instance));
    return instance;
}

static VkSurfaceKHR create_surface(VkInstance vk_handle_instance, GLFWwindow* glfw_window)
{
    VkSurfaceKHR surface;
    VK_CHECK(glfwCreateWindowSurface(vk_handle_instance, glfw_window, nullptr, &surface));
    return surface;
}

static VkPhysicalDevice select_physical_device(VkInstance vk_handle_instance, uint32_t physical_device_ID)
{
    uint32_t count = 0u;
    vkEnumeratePhysicalDevices(vk_handle_instance, &count, nullptr);
    std::vector<VkPhysicalDevice> physical_device_vec(count, VK_NULL_HANDLE);
    vkEnumeratePhysicalDevices(vk_handle_instance, &count, physical_device_vec.data());

    LOG("Available Physical Devices\n");
    for (size_t i = 0u; i < physical_device_vec.size(); i++)
    {
        VkPhysicalDeviceProperties props;
        vkGetPhysicalDeviceProperties(physical_device_vec[i], &props);
        LOG("\t%lu - %s\n", i, props.deviceName);
    }

    if (physical_device_ID >= count)
        physical_device_ID = 0u;

    LOG("Selecting Physical Device %u\n", physical_device_ID);

    return physical_device_vec[physical_device_ID];
}

static uint32_t select_queue_family_index(VkPhysicalDevice vk_handle_physical_device, VkSurfaceKHR vk_handle_surface, VkQueueFlags queue_flags, bool queue_needs_present)
{
    uint32_t count = 0u;
    vkGetPhysicalDeviceQueueFamilyProperties(vk_handle_physical_device, &count, nullptr);
    std::vector<VkQueueFamilyProperties> queue_family_props_vec(count);
    vkGetPhysicalDeviceQueueFamilyProperties(vk_handle_physical_device, &count, queue_family_props_vec.data());

    uint32_t idx = UINT32_MAX;

    for (uint32_t i = 0u; i < count, idx == UINT32_MAX; ++i)
    {
        if ((queue_family_props_vec[i].queueFlags & queue_flags) == queue_flags)
        {
            if (queue_needs_present)
            {
                VkBool32 supports_present{VK_FALSE};
                vkGetPhysicalDeviceSurfaceSupportKHR(vk_handle_physical_device, i, vk_handle_surface, &supports_present);

                if (supports_present)
                    idx = i;
            }
            else
                idx = i;
        }
    }

    assert(idx < UINT32_MAX && "No device queue found that meets requirements.\n");
    return idx;
}

static VkDevice create_device(VkPhysicalDevice vk_handle_physical_device, 
    uint32_t queue_family_idx, 
    void* pnext_chain,
    const std::vector<const char*>& layer_vec, 
    const std::vector<const char*>& extension_vec)
{
    const float queue_priority = 1.0f;

    const VkDeviceQueueCreateInfo queue_create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
        .queueFamilyIndex = queue_family_idx,
        .queueCount = 1u,
        .pQueuePriorities = &queue_priority
    };

    const VkDeviceCreateInfo create_info = {
        .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
        .pNext = pnext_chain,
        .queueCreateInfoCount = 1u,
        .pQueueCreateInfos = &queue_create_info,
        .enabledLayerCount = static_cast<uint32_t>(layer_vec.size()),
        .ppEnabledLayerNames = layer_vec.data(),
        .enabledExtensionCount = static_cast<uint32_t>(extension_vec.size()),
        .ppEnabledExtensionNames = extension_vec.data(),
        .pEnabledFeatures = nullptr
    };

    VkDevice vk_handle_device{VK_NULL_HANDLE};
    VK_CHECK(vkCreateDevice(vk_handle_physical_device, &create_info, nullptr, &vk_handle_device));
    return vk_handle_device;
}

static VkQueue get_queue(VkDevice vk_handle_device, uint32_t queue_family_index)
{
   VkQueue queue = VK_NULL_HANDLE;
   vkGetDeviceQueue(vk_handle_device, queue_family_index, 0, &queue);
   return queue;
}

static std::pair<VkFormat, VkColorSpaceKHR> get_swapchain_image_format_and_color_space(VkPhysicalDevice vk_handle_physical_device, VkSurfaceKHR vk_handle_surface, VkFormat requested_format)
{
    uint32_t count = 0u;
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_handle_physical_device, vk_handle_surface, &count, nullptr));
    std::vector<VkSurfaceFormatKHR> supported_surface_format_vec(count);
    VK_CHECK(vkGetPhysicalDeviceSurfaceFormatsKHR(vk_handle_physical_device, vk_handle_surface, &count, supported_surface_format_vec.data()));

    for (const VkSurfaceFormatKHR& surface_format : supported_surface_format_vec)
        if (surface_format.format == requested_format)
            return {surface_format.format, surface_format.colorSpace};

    assert(false && "Requested Swapchain image format not found!");
    return {};
}

static uint32_t get_swapchain_min_image_count(const VkSurfaceCapabilitiesKHR& surface_capabilities, uint32_t requested_image_count)
{
    assert(requested_image_count > 0 && "Invalid requested image count for swapchain!");

    uint32_t min_image_count = UINT32_MAX;

    // If the maxImageCount is 0, then there is not a limit on the number of images the swapchain
    // can support (ignoring memory constraints). See the Vulkan Spec for more information.

    if (surface_capabilities.maxImageCount == 0)
    {
        if (requested_image_count >= surface_capabilities.minImageCount)
            min_image_count = requested_image_count;
        else
        {
            EXIT("Failed to create Swapchain. The requested number of images %u does not meet the minimum requirement of %u.\n", requested_image_count, surface_capabilities.minImageCount);
        }
    }
    else if (requested_image_count >= surface_capabilities.minImageCount && requested_image_count <= surface_capabilities.maxImageCount)
        min_image_count = requested_image_count;
    else
    {
        EXIT("The number of requested Swapchain images %u is not supported. Min: %u Max: %u.\n", requested_image_count, surface_capabilities.minImageCount, surface_capabilities.maxImageCount);
    }

    return min_image_count;
}

static VkExtent2D get_swapchain_extent(const VkSurfaceCapabilitiesKHR& surface_capabilities, VkExtent2D requested_extent)
{
    // The Vulkan Spec states that if the current width/height is 0xFFFFFFFF, then the surface size
    // will be deteremined by the extent specified in the VkSwapchainCreateInfoKHR.

    if (surface_capabilities.currentExtent.width != (uint32_t)-1)
        return requested_extent;
    return surface_capabilities.currentExtent;
}

static VkSurfaceTransformFlagBitsKHR get_swapchain_pre_transform(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
    if (surface_capabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
        return VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    LOG("WARNING - Swapchain pretransform is not IDENTITIY_BIT_KHR!\n");
    return surface_capabilities.currentTransform;
}

static VkCompositeAlphaFlagBitsKHR get_swapchain_composite_alpha(const VkSurfaceCapabilitiesKHR& surface_capabilities)
{
    VkCompositeAlphaFlagBitsKHR vk_compositeAlpha = VK_COMPOSITE_ALPHA_FLAG_BITS_MAX_ENUM_KHR;

    // Determine the composite alpha format the application needs.
    // Find a supported composite alpha format (not all devices support alpha opaque),
    // but we prefer it.
    // Simply select the first composite alpha format available
    // Used for blending with other windows in the system

    const VkCompositeAlphaFlagBitsKHR vk_compositeAlphaFlags[4] = {
        VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
        VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
        VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
    };

    for (size_t i = 0; i < 4; ++i)
    {
        if (surface_capabilities.supportedCompositeAlpha & vk_compositeAlphaFlags[i]) 
        {
            vk_compositeAlpha = vk_compositeAlphaFlags[i];
            break;
        };
    }

    return vk_compositeAlpha;
}

static VkPresentModeKHR get_swapchain_present_mode(const VkPhysicalDevice vk_handle_physical_device, const VkSurfaceKHR vk_handle_surface, const VkPresentModeKHR requested_present_mode)
{
    uint32_t count = 0;
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_handle_physical_device, vk_handle_surface, &count, nullptr));
    std::vector<VkPresentModeKHR> supported_present_mode_vec(count, VK_PRESENT_MODE_MAX_ENUM_KHR);
    VK_CHECK(vkGetPhysicalDeviceSurfacePresentModesKHR(vk_handle_physical_device, vk_handle_surface, &count, supported_present_mode_vec.data()));

    // Determine the present mode the application needs.
    // Try to use mailbox, it is the lowest latency non-tearing present mode
    // All devices support FIFO (this mode waits for the vertical blank or v-sync)

    for (const VkPresentModeKHR present_mode : supported_present_mode_vec)
    {
        if (present_mode == requested_present_mode)
            return present_mode;
    }

    return VK_PRESENT_MODE_FIFO_KHR;
}

static VkSwapchainCreateInfoKHR populate_swapchain_create_info(VkPhysicalDevice vk_handle_physical_device, VkSurfaceKHR vk_handle_surface, uint32_t requested_min_image_count, VkExtent2D requested_extent, VkFormat requested_image_format, VkPresentModeKHR requested_present_mode)
{
    const auto [format, color_space] = get_swapchain_image_format_and_color_space(vk_handle_physical_device, vk_handle_surface, requested_image_format);

    VkSurfaceCapabilitiesKHR surface_capabilities;
    VK_CHECK(vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vk_handle_physical_device, vk_handle_surface, &surface_capabilities));

    VkSwapchainCreateInfoKHR vk_swapchainCreateInfo {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
        .surface = vk_handle_surface,
        .minImageCount = get_swapchain_min_image_count(surface_capabilities, requested_min_image_count),
        .imageFormat = format,
        .imageColorSpace = color_space,
        .imageExtent = get_swapchain_extent(surface_capabilities, requested_extent),
        .imageArrayLayers = 1,
        .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
        .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
        .queueFamilyIndexCount = 0,
        .pQueueFamilyIndices = nullptr,
        .preTransform = get_swapchain_pre_transform(surface_capabilities),
        .compositeAlpha = get_swapchain_composite_alpha(surface_capabilities),
        .presentMode = get_swapchain_present_mode(vk_handle_physical_device, vk_handle_surface, requested_present_mode),
        .clipped = VK_TRUE,
        .oldSwapchain = VK_NULL_HANDLE,
    };

    return vk_swapchainCreateInfo;
}

static VkSwapchainKHR create_swapchain(const VkDevice vk_handle_device, VkSwapchainCreateInfoKHR& create_info)
{
    const VkSwapchainLatencyCreateInfoNV latency_create_info {
        .sType = VK_STRUCTURE_TYPE_SWAPCHAIN_LATENCY_CREATE_INFO_NV,
        .pNext = nullptr,
        .latencyModeEnable = VK_TRUE,
    };

    create_info.pNext = &latency_create_info;

    VkSwapchainKHR vk_handle_swapchain;
    VK_CHECK(vkCreateSwapchainKHR(vk_handle_device, &create_info, nullptr, &vk_handle_swapchain));
    return vk_handle_swapchain;
}

static std::vector<VkImage> get_swapchain_images(const VkDevice vk_device, const VkSwapchainKHR vk_swapchain)
{
    uint32_t numSwapchainImages = 0;
    std::vector<VkImage> vk_swapchainImages;

    VK_CHECK(vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &numSwapchainImages, nullptr));
    vk_swapchainImages.resize(numSwapchainImages);
    VK_CHECK(vkGetSwapchainImagesKHR(vk_device, vk_swapchain, &numSwapchainImages, vk_swapchainImages.data()));

    return vk_swapchainImages;
}

static std::vector<VkImageView> create_swapchain_image_views(const VkDevice vk_device, const std::vector<VkImage>& vk_swapchainImages, const VkFormat vk_swapchainImageFormat)
{
    std::vector<VkImageView> vk_swapchainImageViews(vk_swapchainImages.size());

    VkImageViewCreateInfo vk_imageViewCreateInfo {
        .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
        .viewType = VK_IMAGE_VIEW_TYPE_2D,
        .format = vk_swapchainImageFormat,
        .components = {
            .r = VK_COMPONENT_SWIZZLE_IDENTITY,
            .g = VK_COMPONENT_SWIZZLE_IDENTITY,
            .b = VK_COMPONENT_SWIZZLE_IDENTITY,
            .a = VK_COMPONENT_SWIZZLE_IDENTITY },
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1 },
    };

    for (size_t i = 0; i < vk_swapchainImages.size(); ++i)
    {
        vk_imageViewCreateInfo.image = vk_swapchainImages[i];
        VK_CHECK(vkCreateImageView(vk_device, &vk_imageViewCreateInfo, nullptr, &vk_swapchainImageViews[i]));
    }

    LOG("Vulkan Info - # Swapchain Images: %lu\n", vk_swapchainImages.size());
    return vk_swapchainImageViews;
}

namespace vk_core
{
static PFN_vkWaitForPresentKHR vkWaitForPresentKHR = VK_NULL_HANDLE;
static PFN_vkGetLatencyTimingsNV vkGetLatencyTimingsNV = VK_NULL_HANDLE;
static PFN_vkSetLatencyMarkerNV vkSetLatencyMarkerNV = VK_NULL_HANDLE;
static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT = VK_NULL_HANDLE;
static PFN_vkCmdBeginDebugUtilsLabelEXT vkCmdBeginDebugUtilsLabelEXT = VK_NULL_HANDLE;;
static PFN_vkCmdEndDebugUtilsLabelEXT vkCmdEndDebugUtilsLabelEXT = VK_NULL_HANDLE;
static PFN_vkGetCalibratedTimestampsKHR vkGetCalibratedTimestampsKHR = VK_NULL_HANDLE;

static VkInstance vk_handle_instance = VK_NULL_HANDLE;
static VkSurfaceKHR vk_handle_surface = VK_NULL_HANDLE;
static VkPhysicalDevice vk_handle_physical_device = VK_NULL_HANDLE;
static VkPhysicalDeviceProperties vk_phys_dev_props;
static VkPhysicalDeviceMemoryProperties vk_phys_dev_mem_props;
static VkDevice vk_handle_device = VK_NULL_HANDLE;
static VkQueue vk_handle_queue = VK_NULL_HANDLE;
static uint32_t queue_family_idx = 0u;
static VkSwapchainKHR vk_handle_swapchain = VK_NULL_HANDLE;
static std::vector<VkImage> vk_handle_swapchain_image_vec;
static std::vector<VkImageView> vk_handle_swapchain_image_view_vec;
static VkFormat vk_format_swapchain_image = VK_FORMAT_UNDEFINED;
static uint32_t active_swapchain_image_idx = 0u;


VkQueryPool create_query_pool(VkQueryType type, uint32_t count, VkQueryPipelineStatisticFlags pipeline_stat_flags, VkQueryPoolCreateFlags query_pool_flags, void* p_next)
{
    const VkQueryPoolCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_QUERY_POOL_CREATE_INFO,
        .pNext = p_next,
        .flags = query_pool_flags,
        .queryType = type,
        .queryCount = count,
        .pipelineStatistics = pipeline_stat_flags,
    };

    VkQueryPool vk_handle {VK_NULL_HANDLE};
    VK_CHECK(vkCreateQueryPool(vk_handle_device, &create_info, nullptr, &vk_handle));
    return vk_handle;
}

void get_query_pool_results(VkQueryPool vk_handle_query_pool, uint32_t first_query, uint32_t query_count, size_t data_size, void* data, VkDeviceSize stride, VkQueryResultFlags flags)
{
    VK_CHECK(vkGetQueryPoolResults(vk_handle_device, vk_handle_query_pool, first_query, query_count, data_size, data, stride, flags));
}

void get_calibrated_timestamps(const VkCalibratedTimestampInfoKHR* timestamp_infos, uint64_t* timestamps, uint32_t count, uint64_t* max_deviation)
{
    if (vkGetCalibratedTimestampsKHR == VK_NULL_HANDLE)
        return;

    VK_CHECK(vkGetCalibratedTimestampsKHR(vk_handle_device, count, timestamp_infos, timestamps, max_deviation));
}


template<typename T>
void load_instance_function(T& func, const char* name)
{
    func = (T)vkGetInstanceProcAddr(vk_handle_instance, name);
    ASSERT(func, "Warning - Failed to load %s\n", name);
}

template<typename T>
void load_device_function(T& func, const char* name)
{
    func = (T)vkGetDeviceProcAddr(vk_handle_device, name);
    ASSERT(func, "Warning - Failed to load %s\n", name);
}

uint32_t get_memory_type_idx(const uint32_t memory_type_indices, const VkMemoryPropertyFlags memory_property_flags)
{
   	// Iterate over all memory types available for the device used in this example
	for (uint32_t i = 0; i < vk_phys_dev_mem_props.memoryTypeCount; i++)
	{
		if (memory_type_indices & (1 << i) && (vk_phys_dev_mem_props.memoryTypes[i].propertyFlags & memory_property_flags) == memory_property_flags)
		{
			return i;
		}
	}

    EXIT("Could not find suitable memory type!");
    return 0; 
}

bool extension_requested(const std::vector<const char*>& extension_vec, const char* requested_extension)
{
    auto it = std::find_if(extension_vec.begin(), extension_vec.end(), [&requested_extension](const char* str) { return strcmp(str, requested_extension) == 0; });
    return (it != extension_vec.end());
}

void init(const InitInfo& init_info)
{
    vk_handle_instance = create_instance(init_info.api_version, init_info.instance_layers, init_info.instance_extensions);
    vk_handle_surface = create_surface(vk_handle_instance, init_info.glfw_window);
    vk_handle_physical_device = select_physical_device(vk_handle_instance, init_info.physical_device_ID);
    queue_family_idx = select_queue_family_index(vk_handle_physical_device, vk_handle_surface, init_info.queue_flags, init_info.queue_needs_present);
    vk_handle_device = create_device(vk_handle_physical_device, queue_family_idx, init_info.device_pnext_chain, init_info.device_layers, init_info.device_extensions);
    vk_handle_queue = get_queue(vk_handle_device, queue_family_idx);

    VkSwapchainCreateInfoKHR swapchain_create_info = populate_swapchain_create_info(vk_handle_physical_device, vk_handle_surface, init_info.swapchain_min_image_count, init_info.swapchain_image_extent, init_info.swapchain_image_format, init_info.swapchain_present_mode);
    vk_handle_swapchain = create_swapchain(vk_handle_device, swapchain_create_info);
    vk_handle_swapchain_image_vec = get_swapchain_images(vk_handle_device, vk_handle_swapchain);
    vk_handle_swapchain_image_view_vec = create_swapchain_image_views(vk_handle_device, vk_handle_swapchain_image_vec, swapchain_create_info.imageFormat);

    vkGetPhysicalDeviceProperties(vk_handle_physical_device, &vk_phys_dev_props);

    // Also need to check if correct features are enabled!!!

    if (extension_requested(init_info.instance_extensions, VK_EXT_DEBUG_UTILS_EXTENSION_NAME))
    {
        load_instance_function<PFN_vkSetDebugUtilsObjectNameEXT>(vkSetDebugUtilsObjectNameEXT, "vkSetDebugUtilsObjectNameEXT");
        load_instance_function<PFN_vkCmdBeginDebugUtilsLabelEXT>(vkCmdBeginDebugUtilsLabelEXT, "vkCmdBeginDebugUtilsLabelEXT");
        load_instance_function<PFN_vkCmdEndDebugUtilsLabelEXT>(vkCmdEndDebugUtilsLabelEXT, "vkCmdEndDebugUtilsLabelEXT");
    }

    if (extension_requested(init_info.device_extensions, VK_KHR_PRESENT_WAIT_EXTENSION_NAME))
    {
        load_device_function<PFN_vkWaitForPresentKHR>(vkWaitForPresentKHR, "vkWaitForPresentKHR");
    }

    if (extension_requested(init_info.device_extensions, VK_NV_LOW_LATENCY_2_EXTENSION_NAME))
    {
        load_device_function<PFN_vkGetLatencyTimingsNV>(vkGetLatencyTimingsNV, "vkGetLatencyTimingsNV");
        load_device_function<PFN_vkSetLatencyMarkerNV>(vkSetLatencyMarkerNV, "vkSetLatencyMarkerNV");
    }

    if (extension_requested(init_info.device_extensions, VK_KHR_CALIBRATED_TIMESTAMPS_EXTENSION_NAME))
    {
        load_device_function<PFN_vkGetCalibratedTimestampsKHR>(vkGetCalibratedTimestampsKHR, "vkGetCalibratedTimestampsKHR");
    }
}

void terminate()
{
    for (uint32_t i = 0; i < vk_handle_swapchain_image_vec.size(); i++)
    {
        vkDestroyImageView(vk_handle_device, vk_handle_swapchain_image_view_vec[i], nullptr);
    }

    vkDestroySwapchainKHR(vk_handle_device, vk_handle_swapchain, nullptr);
    vkDestroyDevice(vk_handle_device, nullptr);
    vkDestroySurfaceKHR(vk_handle_instance, vk_handle_surface, nullptr);
    vkDestroyInstance(vk_handle_instance, nullptr);
}


VkInstance get_instance() { return vk_handle_instance; }
VkPhysicalDevice get_physical_device() { return vk_handle_physical_device; }
VkDevice get_device() { return vk_handle_device; }
VkQueue get_queue() { return vk_handle_queue; }

int32_t get_swapchain_image_count()
{
    return static_cast<int32_t>(vk_handle_swapchain_image_vec.size());
}

VkImage get_swapchain_image(int32_t idx)
{
    assert(idx < vk_handle_swapchain_image_vec.size());
    assert(-1 < idx);
    return vk_handle_swapchain_image_vec[idx];
}


const VkPhysicalDeviceProperties& get_physical_device_properties()
{
    return vk_phys_dev_props;
}

void present(uint32_t swapchain_image_idx, std::vector<VkSemaphore>&& vk_handle_wait_sem4_vec, void* p_next)
{
    const VkPresentInfoKHR present_info {
        .sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
        .pNext = p_next,
        .waitSemaphoreCount = static_cast<uint32_t>(vk_handle_wait_sem4_vec.size()),
        .pWaitSemaphores = vk_handle_wait_sem4_vec.data(),
        .swapchainCount = 1u,
        .pSwapchains = &vk_handle_swapchain,
        .pImageIndices = &swapchain_image_idx,
        .pResults = nullptr,
    };

    VK_CHECK(vkQueuePresentKHR(vk_handle_queue, &present_info));
}

void wait_for_present(uint64_t present_id, uint64_t timeout)
{
    VK_CHECK(vkWaitForPresentKHR(vk_handle_device, vk_handle_swapchain, present_id, timeout));
}

void device_wait_idle()
{
    VK_CHECK(vkDeviceWaitIdle(vk_handle_device));
}

void queue_submit(const VkSubmitInfo& submit_info, VkFence vk_handle_signal_fence)
{
    VK_CHECK(vkQueueSubmit(vk_handle_queue, 1u, &submit_info, vk_handle_signal_fence));
}

void queue_wait_idle()
{
    VK_CHECK(vkQueueWaitIdle(vk_handle_queue));
}


VkSemaphore create_semaphore(VkSemaphoreCreateFlags flags)
{
    const VkSemaphoreCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
    };

    VkSemaphore vk_handle_sem4 = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSemaphore(vk_handle_device, &create_info, nullptr, &vk_handle_sem4));
    return vk_handle_sem4;
}

void destroy_semaphore(VkSemaphore vk_handle_sem4)
{
    vkDestroySemaphore(vk_handle_device, vk_handle_sem4, nullptr);
}

VkFence create_fence(VkFenceCreateFlags flags)
{
    const VkFenceCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
    };

    VkFence vk_handle_fence = VK_NULL_HANDLE;
    VK_CHECK(vkCreateFence(vk_handle_device, &create_info, nullptr, &vk_handle_fence));
    return vk_handle_fence;
}

VkFence create_fence(const char* name, VkFenceCreateFlags flags)
{
    const VkFence vk_handle_fence = create_fence(flags);

    const VkDebugUtilsObjectNameInfoEXT debug_info {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = VK_OBJECT_TYPE_FENCE,
        .objectHandle = reinterpret_cast<uint64_t>(vk_handle_fence),
        .pObjectName = name,
    };

    if (vkSetDebugUtilsObjectNameEXT)
    {
        VK_CHECK(vkSetDebugUtilsObjectNameEXT(vk_handle_device, &debug_info));
    }

    return vk_handle_fence;
}

void wait_for_fence(VkFence vk_handle_fence, uint64_t timeout)
{
    VK_CHECK(vkWaitForFences(vk_handle_device, 1u, &vk_handle_fence, VK_TRUE, timeout));
}

void reset_fence(VkFence vk_handle_fence)
{
    VK_CHECK(vkResetFences(vk_handle_device, 1u, &vk_handle_fence)); 
}

VkCommandPool create_command_pool(VkCommandPoolCreateFlags flags)
{
    const VkCommandPoolCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
        .pNext = nullptr,
        .flags = flags,
        .queueFamilyIndex = queue_family_idx,
    };

    VkCommandPool vk_handle_cmd_pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateCommandPool(vk_handle_device, &create_info, nullptr, &vk_handle_cmd_pool));
    return vk_handle_cmd_pool;
}

void reset_command_pool(VkCommandPool vk_handle_cmd_pool)
{
    VK_CHECK(vkResetCommandPool(vk_handle_device, vk_handle_cmd_pool, 0x0));
}

void destroy_command_pool(VkCommandPool vk_handle_cmd_pool)
{
    vkDestroyCommandPool(vk_handle_device, vk_handle_cmd_pool, nullptr);
}

VkCommandBuffer allocate_command_buffer(VkCommandPool cmd_pool, VkCommandBufferLevel level)
{
    const VkCommandBufferAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
        .pNext = nullptr,
        .commandPool = cmd_pool,
        .level = level,
        .commandBufferCount = 1u,
    };

    VkCommandBuffer vk_handle_cmd_buff = VK_NULL_HANDLE;
    VK_CHECK(vkAllocateCommandBuffers(vk_handle_device, &alloc_info, &vk_handle_cmd_buff));
    return vk_handle_cmd_buff;
}

VkCommandBuffer allocate_command_buffer(const char* name, VkCommandPool cmd_pool, VkCommandBufferLevel level)
{
    const VkCommandBuffer vk_handle_cmd_buff = allocate_command_buffer(cmd_pool, level);

    const VkDebugUtilsObjectNameInfoEXT debug_info {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .pNext = nullptr,
        .objectType = VK_OBJECT_TYPE_COMMAND_BUFFER,
        .objectHandle = reinterpret_cast<uint64_t>(vk_handle_cmd_buff),
        .pObjectName = name,
    };

    if (vkSetDebugUtilsObjectNameEXT)
    {
        VK_CHECK(vkSetDebugUtilsObjectNameEXT(vk_handle_device, &debug_info));
    }

    return vk_handle_cmd_buff;
}


void begin_command_buffer(VkCommandBuffer vk_handle_cmd_buff, VkCommandBufferUsageFlags flags)
{
    const VkCommandBufferBeginInfo begin_info {
        .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
        .flags = flags
    };

    VK_CHECK(vkBeginCommandBuffer(vk_handle_cmd_buff, &begin_info));
}

void end_command_buffer(VkCommandBuffer vk_handle_cmd_buff)
{
    VK_CHECK(vkEndCommandBuffer(vk_handle_cmd_buff));
}







VkImageView get_swapchain_image_view(uint32_t idx)
{
    assert(idx < vk_handle_swapchain_image_view_vec.size());
    return vk_handle_swapchain_image_view_vec[idx];
}

uint32_t acquire_next_swapchain_image(VkSemaphore vk_handle_signal_sem4, VkFence vk_handle_signal_fence)
{
    uint32_t image_idx = 0u;
    VK_CHECK(vkAcquireNextImageKHR(vk_handle_device, vk_handle_swapchain, UINT64_MAX, vk_handle_signal_sem4, vk_handle_signal_fence, &image_idx));
    return image_idx;
}



void wait_for_fences(const uint32_t fence_count, const VkFence* vk_handle_fence_list, const VkBool32 wait_all, const uint64_t timeout)
{
    VK_CHECK(vkWaitForFences(vk_handle_device, fence_count, vk_handle_fence_list, wait_all, timeout));
}

void reset_fences(const uint32_t fence_count, const VkFence* vk_handle_fence_list)
{
    VK_CHECK(vkResetFences(vk_handle_device, fence_count, vk_handle_fence_list)); 
}

void destroy_fence(const VkFence vk_handle_fence)
{
    vkDestroyFence(vk_handle_device, vk_handle_fence, nullptr);
}


void debug_utils_begin_label(VkCommandBuffer vk_handle_cmd_buff, const char* name)
{
    if (vkCmdBeginDebugUtilsLabelEXT == VK_NULL_HANDLE)
        return;

    const VkDebugUtilsLabelEXT debug_label {
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_LABEL_EXT,
        .pNext = nullptr,
        .pLabelName = name,
    };

    vkCmdBeginDebugUtilsLabelEXT(vk_handle_cmd_buff, &debug_label);
}

void debug_utils_end_label(VkCommandBuffer vk_handle_cmd_buff)
{
    if (vkCmdEndDebugUtilsLabelEXT == VK_NULL_HANDLE)
        return;

    vkCmdEndDebugUtilsLabelEXT(vk_handle_cmd_buff);
}

void get_latency_timings_NV(VkGetLatencyMarkerInfoNV* latency_marker_info)
{
    if (vkGetLatencyTimingsNV == VK_NULL_HANDLE)
        return;

    vkGetLatencyTimingsNV(vk_handle_device, vk_handle_swapchain, latency_marker_info);
}

void set_latency_marker_NV(uint64_t present_id, VkLatencyMarkerNV marker)
{
    if (vkSetLatencyMarkerNV == VK_NULL_HANDLE)
        return;

    const VkSetLatencyMarkerInfoNV info {
        .sType = VK_STRUCTURE_TYPE_SET_LATENCY_MARKER_INFO_NV,
        .pNext = nullptr,
        .presentID = present_id,
        .marker = marker,
    };

    vkSetLatencyMarkerNV(vk_handle_device, vk_handle_swapchain, &info);
}



VkSampler create_sampler(const VkSamplerCreateInfo& create_info)
{
    VkSampler vk_handle_sampler = VK_NULL_HANDLE;
    VK_CHECK(vkCreateSampler(vk_handle_device, &create_info, nullptr, &vk_handle_sampler));
    return vk_handle_sampler;;
}

VkImage create_image(const VkImageCreateInfo& create_info)
{
    VkImage image = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImage(vk_handle_device, &create_info, nullptr, &image));
    return image;
}

VkImageView create_image_view(const VkImageViewCreateInfo& create_info)
{
    VkImageView image_view = VK_NULL_HANDLE;
    VK_CHECK(vkCreateImageView(vk_handle_device, &create_info, nullptr, &image_view));
    return image_view;
}

VkDeviceMemory allocate_image_memory(const VkImage vk_handle_image, const VkMemoryPropertyFlags flags)
{
    VkMemoryRequirements memory_requirements;
    vkGetImageMemoryRequirements(vk_handle_device, vk_handle_image, &memory_requirements);

    const VkMemoryAllocateInfo memory_alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = get_memory_type_idx(memory_requirements.memoryTypeBits, flags),
    };

    VkDeviceMemory vk_image_memory = VK_NULL_HANDLE;
    vkAllocateMemory(vk_handle_device, &memory_alloc_info, nullptr, &vk_image_memory);
    return vk_image_memory;
}

void bind_image_memory(const VkImage vk_handle_image, const VkDeviceMemory vk_handle_image_memory)
{
    VK_CHECK(vkBindImageMemory(vk_handle_device, vk_handle_image, vk_handle_image_memory, 0));
}

void destroy_image(const VkImage vk_handle_image)
{
    vkDestroyImage(vk_handle_device, vk_handle_image, nullptr);
}

void destroy_image_view(const VkImageView vk_handle_image_view)
{
    vkDestroyImageView(vk_handle_device, vk_handle_image_view, nullptr);
}

VkBuffer create_buffer(const VkBufferCreateInfo& create_info)
{
    VkBuffer vk_handle_buffer = VK_NULL_HANDLE;
    VK_CHECK(vkCreateBuffer(vk_handle_device, &create_info, nullptr, &vk_handle_buffer));
    return vk_handle_buffer;
}

VkDeviceMemory allocate_buffer_memory(const VkBuffer vk_handle_buffer, const VkMemoryPropertyFlags flags, VkDeviceSize& size)
{
    VkMemoryRequirements memory_requirements;
    vkGetBufferMemoryRequirements(vk_handle_device, vk_handle_buffer, &memory_requirements);

    const VkMemoryAllocateInfo memory_alloc_info{
        .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
        .pNext = nullptr,
        .allocationSize = memory_requirements.size,
        .memoryTypeIndex = get_memory_type_idx(memory_requirements.memoryTypeBits, flags),
    };

    VkDeviceMemory vk_buffer_memory = VK_NULL_HANDLE;
    vkAllocateMemory(vk_handle_device, &memory_alloc_info, nullptr, &vk_buffer_memory);

    size = memory_requirements.size;
    return vk_buffer_memory;
}

void bind_buffer_memory(const VkBuffer vk_handle_buffer, const VkDeviceMemory vk_handle_buffer_memory)
{
    VK_CHECK(vkBindBufferMemory(vk_handle_device, vk_handle_buffer, vk_handle_buffer_memory, 0));

}

void destroy_buffer(const VkBuffer vk_handle_buffer)
{
    vkDestroyBuffer(vk_handle_device, vk_handle_buffer, nullptr);
}

VkShaderModule create_shader_module(const VkShaderModuleCreateInfo& create_info)
{
    VkShaderModule vk_handle_shader_module = VK_NULL_HANDLE;
    VK_CHECK(vkCreateShaderModule(vk_handle_device, &create_info, NULL, &vk_handle_shader_module));
    return vk_handle_shader_module;
}

void destroy_shader_module(const VkShaderModule vk_handle_shader_module)
{
    vkDestroyShaderModule(vk_handle_device, vk_handle_shader_module, nullptr);
}


VkPipeline create_graphics_pipeline(const VkGraphicsPipelineCreateInfo& create_info)
{
    VkPipeline vk_handle_pipeline = VK_NULL_HANDLE;
    VK_CHECK(vkCreateGraphicsPipelines(vk_handle_device, VK_NULL_HANDLE, 1, &create_info, nullptr, &vk_handle_pipeline));
    return vk_handle_pipeline;
}

void destroy_pipeline(const VkPipeline vk_handle_pipeline)
{
    vkDestroyPipeline(vk_handle_device, vk_handle_pipeline, nullptr);
}



VkDescriptorPool create_desc_pool(const VkDescriptorPoolCreateInfo& create_info)
{
    VkDescriptorPool vk_handle_desc_pool = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorPool(vk_handle_device, &create_info, nullptr, &vk_handle_desc_pool));
    return vk_handle_desc_pool;
}

void destroy_desc_pool(const VkDescriptorPool vk_handle_desc_pool)
{
    vkDestroyDescriptorPool(vk_handle_device, vk_handle_desc_pool, nullptr);
}

VkDescriptorSetLayout create_desc_set_layout(const VkDescriptorSetLayoutCreateInfo& create_info)
{
    VkDescriptorSetLayout vk_handle_desc_set_layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreateDescriptorSetLayout(vk_handle_device, &create_info, nullptr, &vk_handle_desc_set_layout));
    return vk_handle_desc_set_layout;
}

void destroy_desc_set_layout(const VkDescriptorSetLayout vk_handle_desc_set_layout)
{
    vkDestroyDescriptorSetLayout(vk_handle_device, vk_handle_desc_set_layout, nullptr);
}

std::vector<VkDescriptorSet> allocate_desc_sets(const VkDescriptorSetAllocateInfo& alloc_info)
{
    std::vector<VkDescriptorSet> vk_handle_desc_set_list(alloc_info.descriptorSetCount, VK_NULL_HANDLE);
    VK_CHECK(vkAllocateDescriptorSets(vk_handle_device, &alloc_info, vk_handle_desc_set_list.data()));
    return vk_handle_desc_set_list;
}

void update_desc_sets(const uint32_t update_count, const VkWriteDescriptorSet* const p_write_desc_set_list, const uint32_t copy_count, const VkCopyDescriptorSet* const p_copy_desc_set_list)
{
    vkUpdateDescriptorSets(vk_handle_device, update_count, p_write_desc_set_list, copy_count, p_copy_desc_set_list);
}

VkPipelineLayout create_pipeline_layout(const VkPipelineLayoutCreateInfo& create_info)
{
    VkPipelineLayout vk_handle_pipeline_layout = VK_NULL_HANDLE;
    VK_CHECK(vkCreatePipelineLayout(vk_handle_device, &create_info, nullptr, &vk_handle_pipeline_layout));
    return vk_handle_pipeline_layout;
}

void destroy_pipeline_layout(VkPipelineLayout vk_handle_pipeline_layout)
{
    vkDestroyPipelineLayout(vk_handle_device, vk_handle_pipeline_layout, nullptr);
}

void map_memory(const VkDeviceMemory vk_handle_memory, const VkDeviceSize offset, const VkDeviceSize size, const VkMemoryMapFlags flags, void** data)
{
    VK_CHECK(vkMapMemory(vk_handle_device, vk_handle_memory, offset, size, flags, data));
}

void unmap_memory(const VkDeviceMemory vk_handle_memory)
{
    vkUnmapMemory(vk_handle_device, vk_handle_memory);
}

void free_memory(const VkDeviceMemory vk_handle_memory)
{
    vkFreeMemory(vk_handle_device, vk_handle_memory, nullptr);
}


void queue_submit(const uint32_t submit_count, const VkSubmitInfo* const p_submit_infos, const VkFence vk_handle_signal_fence)
{
    vkQueueSubmit(vk_handle_queue, submit_count, p_submit_infos, vk_handle_signal_fence);
}



uint32_t get_queue_family_idx()
{
    return queue_family_idx;
}

VkImage get_active_swapchain_image()
{
    return vk_handle_swapchain_image_vec[active_swapchain_image_idx];
}

VkImageMemoryBarrier get_active_swapchain_image_memory_barrier(const VkAccessFlags src_access_flags, const VkAccessFlags dst_access_flags, const VkImageLayout old_layout, const VkImageLayout new_layout)
{
    const VkImageMemoryBarrier barrier {
        .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
        .pNext = nullptr,
        .srcAccessMask = src_access_flags,
        .dstAccessMask = dst_access_flags,
        .oldLayout = old_layout, 
        .newLayout = new_layout,
        .srcQueueFamilyIndex = queue_family_idx,
        .dstQueueFamilyIndex = queue_family_idx,
        .image = vk_handle_swapchain_image_vec[active_swapchain_image_idx],
        .subresourceRange = {
            .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
            .baseMipLevel = 0,
            .levelCount = 1,
            .baseArrayLayer = 0,
            .layerCount = 1,
        }
    };

    return barrier;
}

}; // vk_core