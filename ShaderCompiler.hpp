#include <array>
#include <vector>
#include <string>
#include <iostream>

#include <vulkan/vulkan.h>

#include <map>
#include <unordered_map>

struct DescriptorBindingReflection
{
    uint32_t binding_ID; 
    VkDescriptorType descriptor_type;
    uint32_t         descriptorCount;

    uint32_t set_ID;
    std::unordered_map<std::string, void*> m_internal_var_reflection_umap;

    DescriptorBindingReflection(DescriptorBindingReflection&& other) noexcept
        : m_internal_var_reflection_umap(std::move(other.m_internal_var_reflection_umap))
    {

    }
};

struct DescriptorSetLayoutReflection
{
private:
    std::unordered_map<std::string, DescriptorBindingReflection> m_resource_umap;
public:
    const DescriptorBindingReflection& get_resource(const std::string& resource_name) const;
};

struct DescriptorSet
{
private:
    VkDescriptorSet m_vk_handle_desc_set;
    std::vector<bool> m_bindings_written;

    const DescriptorSetLayout* const m_desc_set_layout;
public:
    // explicit DescriptorSet(const DescriptorSetLayout* desc_set_layout);

    void write(const std::string& resource_name, const VkDescriptorBufferInfo& desc_info)
    {

    }
};

struct DescriptorBindingWrite
{
    uint32_t                         dst_set_ID;
    uint32_t                         dst_binding_ID;
    uint32_t                         dst_array_element;
    std::vector<VkDescriptorBufferInfo> buffer_info_vec;
    std::vector<VkDescriptorImageInfo> image_info_vec;
    std::vector<VkBufferView> texel_buffer_view_vec;
};

constexpr uint32_t FRAMES_IN_FLIGHT = 1;

#if 0
    const std::array<VkDescriptorSet, 3> vk_handle_desc_set_array {VK_NULL_HANDLE, VK_NULL_HANDLE, VK_NULL_HANDLE};

    // Index into vk_handle_desc_set_array indicating the first set that is unique
    // to the pipeline. Often, rendering passes have a global set that is shared
    // across all pipelines in a frame (set 0). In such a design, the responsibility
    // index might be 1, indicating that this pipeline is not responsible for 
    // maininting a reference to set 0 (set 0 will be externall bound).
    const uint8_t responsibility_index = 0u;

    // The number of descriptor sets that are owned by this pipeline. 
    // If a pipeline has a responsibility index of 0, then the local_desc_set_count will be at most 3.
    // If a pipeline has a responsibility index of 1, then the local_desc_set_count will be at most 2.
    // If a pipeline has a responsibility index of 2, then the local_desc_set_count will be at most 1.
    // If a pipeline has a responsibility index of 3, then the local_desc_set_count will be 0. Meaning
    // the pipeline is responsible for binding 0 descriptor sets. All descriptors sets are expected
    // to be externally bound.
    const uint8_t local_desc_set_count = 0u;

    // Used to compare against a similar array for already bound descriptor set layout hashes. Indices
    // below the responsibility index can be compared to ensure correct behavior.
    const std::array<uint32_t, 3> debug_desc_set_layout_hash_array {0u, 0u, 0u};

    void bind_responsible_descriptor_sets(VkCommandBuffer vk_handle_command_buffer) const
    {
        vkCmdBindDescriptorSets(vk_handle_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_handle_pipeline_layout, responsibility_index, local_desc_set_count, vk_handle_desc_set_array.data() + responsibility_index, 0, nullptr);
    }
#endif


struct Program
{
private:
    const VkPipeline vk_handle_pipeline {VK_NULL_HANDLE};
    const VkPipelineLayout vk_handle_pipeline_layout {VK_NULL_HANDLE};
    const VkPipelineBindPoint vk_pipeline_bind_point {VK_PIPELINE_BIND_POINT_MAX_ENUM};
    const std::vector<std::vector<VkDescriptorSet>> m_vk_handle_desc_set_vec;
public:
    Program() = default;

    void bind(VkCommandBuffer vk_handle_command_buffer) const
    {
        vkCmdBindPipeline(vk_handle_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_handle_pipeline);
    }

    void bind_descriptor_sets(VkCommandBuffer vk_handle_command_buffer, uint8_t frame_resource_idx) const
    {
        std::vector<VkDescriptorSet> vk_handle_frame_desc_set_vec;

        for (const auto& desc_set_vec : m_vk_handle_desc_set_vec)
        {
            vk_handle_frame_desc_set_vec.push_back(desc_set_vec[frame_resource_idx % static_cast<uint8_t>(desc_set_vec.size())]);
        }

        vkCmdBindDescriptorSets(vk_handle_command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vk_handle_pipeline_layout, 0, 
                                static_cast<uint32_t>(vk_handle_frame_desc_set_vec.size()), vk_handle_frame_desc_set_vec.data(),
                                0, nullptr);
    }
};

struct Compiler_GrahpicsPipeline
{
private:
public:
    void set_desc_set(uint32_t set, VkDescriptorSet vk_handle_desc_set);
    void set_own_set_set();

    // Will set...
    // -- Vertex Input State
    // -- Pipeline Layout
    void set_shaders(const std::vector<std::string>& shader_path_vec);

    void set_vertex_input_state();
    void set_input_assembly_state();
    void set_tessellation_state();
    void set_viewport_state();
    void set_rasterization_state();
    void set_multisample_state();
    void set_depth_stencil_state();
    void set_color_blend_state();

    // GraphicsPipeline compile();
};

struct Compiler_ComputeProgram
{
private:
    std::string m_shader_root_dir;
    // std::array<VkDescriptorPool, FRAMES_IN_FLIGHT> m_vk_handle_desc_pool_array;
    VkDescriptorPool m_vk_handle_desc_pool;

    // Genereated by set_shaders (reflection)
    std::unordered_map<std::string, std::pair<DescriptorBindingReflection, bool>> m_desc_binding_name_to_reflection_umap;
    std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> m_desc_set_id_to_layout_bindings_umap;

    // std::vector<DescriptorBindingWrite> m_pending_desc_binding_write_vec;
    std::array<std::vector<DescriptorBindingWrite>, 3> m_pending_desc_writes_LUT;
    std::vector<std::vector<VkDescriptorSet>> m_vk_handle_desc_set_vec;

    bool all_resources_written() const;
    std::vector<VkDescriptorSetLayout> create_descriptor_set_layouts();
public:
    void set_shader_root_dir(const std::string& shader_root_dir);
    void set_descriptor_pool(const std::vector<VkDescriptorPool>& vk_handle_desc_pool_vec);

    void set_shaders(const char** shader_path_array, uint32_t shader_count);
    void set_shaders(const std::vector<std::string>& shader_path_vec);

    void inherit_descriptor_set(uint32_t set, std::vector<VkDescriptorSet> vk_handle_desc_set_vec);

    void write_descriptor_resource(const std::string& resource_name, const std::vector<VkDescriptorBufferInfo>& desc_info, uint32_t array_idx = 0u);

    Program compile();
};



std::vector<uint32_t> load_spirv_binary(std::string_view shader_path)
{
    return {};
}

std::map<uint32_t, DescriptorSetLayoutReflection> reflect_spirv(std::vector<uint32_t> spirv_binary)
{
    VkDescriptorSetLayoutBinding;
    uint32_t              binding;
    VkDescriptorType      descriptorType;
    uint32_t              descriptorCount;
    VkShaderStageFlags    stageFlags;
    const VkSampler*      pImmutableSamplers;

    std::map<uint32_t, VkDescriptorSetLayoutBinding> desc_set_layout_binding_omap;

    return {};
} 

void merge_reflections(
    std::map<uint32_t, DescriptorSetLayoutReflection>&& shader_reflection, 
    std::map<uint32_t, DescriptorSetLayoutReflection>& program_reflection)
{
} 

void Compiler_ComputeProgram::set_shaders(const std::vector<std::string>& shader_path_vec)
{
    m_desc_binding_name_to_reflection_umap.clear();
    m_desc_set_id_to_layout_bindings_umap.clear();

    // Creating Descriptor Set Layout
    for (const auto& shader_path : shader_path_vec)
    {
        // Load SPRIV Binary
        const auto full_shader_path = m_shader_root_dir + shader_path;
        const auto shader_spirv = load_spirv_binary(full_shader_path);

        // Relfect SPIRV
        auto shader_reflection_omap = reflect_spirv(shader_spirv);

        // Merge Shader Reflection with Program Reflection
        // merge_reflections(std::move(shader_reflection_omap), program_reflection_omap);
    }
}

void Compiler_ComputeProgram::write_descriptor_resource(const std::string& resource_name, const std::vector<VkDescriptorBufferInfo>& desc_info, uint32_t array_idx)
{
    auto iter = m_desc_binding_name_to_reflection_umap.find(resource_name);

    if (iter == m_desc_binding_name_to_reflection_umap.end())
    {
        // Error: Resource not found
        return;
    }

    if (iter->second.first.set_ID > 3)
    {
        // Error: Set ID out of bounds
        return;
    }

    DescriptorBindingWrite write {
        .dst_set_ID = iter->second.first.set_ID,
        .dst_binding_ID = iter->second.first.binding_ID,
        .dst_array_element = array_idx,
        .buffer_info_vec = desc_info
    };

    m_pending_desc_writes_LUT[iter->second.first.set_ID].push_back(write);

    // m_pending_desc_binding_write_vec.push_back(write);

    iter->second.second = true;
}

bool Compiler_ComputeProgram::all_resources_written() const
{
    for (const auto& [name, binding_info] : m_desc_binding_name_to_reflection_umap)
    {
        if (binding_info.second == false)
        {
            return false;
        }
    }

    return true;
}

std::vector<VkDescriptorSetLayout> Compiler_ComputeProgram::create_descriptor_set_layouts()
{
    std::vector<VkDescriptorSetLayout> vk_handle_desc_set_layout_vec;
    for (const auto& [set_ID, layout_bindings] : m_desc_set_id_to_layout_bindings_umap)
    {
        VkDescriptorSetLayoutCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .bindingCount = static_cast<uint32_t>(layout_bindings.size()),
            .pBindings = layout_bindings.data(),
        };

        VkDescriptorSetLayout vk_handle_desc_set_layout;
        vkCreateDescriptorSetLayout(VK_NULL_HANDLE, &create_info, nullptr, &vk_handle_desc_set_layout);
        vk_handle_desc_set_layout_vec.push_back(vk_handle_desc_set_layout);
    }

    return vk_handle_desc_set_layout_vec;
}

Program Compiler_ComputeProgram::compile()
{
    if (!all_resources_written())
    {
        std::cerr << "Program Compilation Failed: Not all resources have been written to." << std::endl;
        return {};
    }

    const std::vector<VkDescriptorSetLayout> vk_handle_desc_set_layout_vec = create_descriptor_set_layouts();

    size_t num_unique_sets_required = 0u;
    for (const auto& pending_desc_write_vec : m_pending_desc_writes_LUT)
    {
        for (const auto& pending_desc_write : pending_desc_write_vec)
        {
            num_unique_sets_required = std::max(num_unique_sets_required, pending_desc_write.buffer_info_vec.size());
        }
    }

    for (uint32_t set_idx = 0u; set_idx < 3; set_idx++)
    {
        if (m_pending_desc_writes_LUT[set_idx].empty())
        {
            break;
        }

        size_t num_unique_sets_required = 0u;
        for (const auto& pending_desc_write : m_pending_desc_writes_LUT[set_idx])
        {
            num_unique_sets_required = std::max(num_unique_sets_required, pending_desc_write.buffer_info_vec.size());
        }

        const VkDescriptorSetAllocateInfo desc_set_alloc_info {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .pNext = nullptr,
            .descriptorPool = m_vk_handle_desc_pool,
            .descriptorSetCount = num_unique_sets_required,
            .pSetLayouts = &vk_handle_desc_set_layout_vec[set_idx]
        };

        std::vector<VkDescriptorSet> vk_handle_desc_set(num_unique_sets_required, VK_NULL_HANDLE);
        vkAllocateDescriptorSets(VK_NULL_HANDLE, &desc_set_alloc_info, vk_handle_desc_set.data());
        m_vk_handle_desc_set_vec.push_back(vk_handle_desc_set);
    }

    // Write

    return {};
}













