#include <vulkan/vulkan.h>

#include <iostream>

#include <string>
#include <vector>
#include <array>
#include <unordered_map>

constexpr uint32_t MAX_DESC_SET = 3u;
constexpr uint32_t FRAME_RESOURCE_COUNT = 3u;

/*  */

class Compiler_DescriptorSetLayout;

struct DescriptorSetLayout
{
private:
    friend class Compiler_DescriptorSetLayout;

    DescriptorSetLayout(VkDescriptorSetLayout _vk_handle, const std::unordered_map<std::string, VkDescriptorSetLayoutBinding>& umap);
public:
    const VkDescriptorSetLayout vk_handle {VK_NULL_HANDLE};
    const std::unordered_map<std::string, VkDescriptorSetLayoutBinding> name_binding_umap;
};

DescriptorSetLayout::DescriptorSetLayout(VkDescriptorSetLayout _vk_handle, const std::unordered_map<std::string, VkDescriptorSetLayoutBinding>& umap)
    : vk_handle{_vk_handle}
    , name_binding_umap{umap}
{}

/*  */

struct Compiler_DescriptorSetLayout
{
private:
    std::unordered_map<std::string, VkDescriptorSetLayoutBinding> name_binding_umap;
public:
    void register_binding(std::string name, uint32_t ID, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage_flags);
    DescriptorSetLayout compile();
};

void Compiler_DescriptorSetLayout::register_binding(std::string name, uint32_t ID, VkDescriptorType type, uint32_t count, VkShaderStageFlags stage_flags)    
{
    name_binding_umap[name] = {
        .binding = ID,
        .descriptorType = type,
        .descriptorCount = count,
        .stageFlags = stage_flags,
        .pImmutableSamplers = nullptr
    };
}
    
DescriptorSetLayout Compiler_DescriptorSetLayout::compile()
{
    std::vector<VkDescriptorSetLayoutBinding> binding_vec;
    binding_vec.reserve(name_binding_umap.size());

    for (const auto [name, binding] : name_binding_umap)
        binding_vec.push_back(binding);

    const VkDescriptorSetLayoutCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .bindingCount = static_cast<uint32_t>(binding_vec.size()),
        .pBindings = binding_vec.data(),
    };

    VkDescriptorSetLayout vk_handle{VK_NULL_HANDLE};
    // vkCreateDescriptorSetLayout();
    return {vk_handle, name_binding_umap};
}

/*  */

struct Compiler_DescriptorSet
{
private:
    const VkDescriptorPool vk_handle_pool;
    const DescriptorSetLayout& layout;

    std::unordered_map<std::string, VkDescriptorBufferInfo> queued_buffer_writes;
    std::unordered_map<std::string, VkDescriptorImageInfo>  queued_image_writes;

    bool sanity_check();
    VkDescriptorSet alloc_set();
    std::vector<VkWriteDescriptorSet> generate_writes(VkDescriptorSet vk_handle_desc_set);

public:
    explicit Compiler_DescriptorSet(VkDescriptorPool _vk_handle_pool, const DescriptorSetLayout& _layout);

    void write(std::string name, VkDescriptorBufferInfo&& info);
    void write(std::string name, VkDescriptorImageInfo&& info);
    VkDescriptorSet compile();
};

bool Compiler_DescriptorSet::sanity_check()
{
    const size_t total_write_count = queued_buffer_writes.size() + queued_image_writes.size();

    if (total_write_count != layout.name_binding_umap.size())
    {
        std::cerr << "Descriptor Set compilation FAILED. Not all bindings have been written to. " 
                  << "The following bindings need to be written to for compilation to succeed...\n";

        for (const auto [name, info] : layout.name_binding_umap)
        {
            if (!queued_buffer_writes.contains(name) && !queued_image_writes.contains(name))
            {
                std::cerr << '\t' << name << '\n';
            }
        }

        return false;
    }
    return true;
}

VkDescriptorSet Compiler_DescriptorSet::alloc_set()
{
    const VkDescriptorSetAllocateInfo alloc_info {
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = nullptr,
        .descriptorPool = vk_handle_pool,
        .descriptorSetCount = 1u,
        .pSetLayouts = &layout.vk_handle,
    };

    VkDescriptorSet vk_handle_desc_set{VK_NULL_HANDLE};
    // vkAllocateDescriptorSets()

    return vk_handle_desc_set;
}

std::vector<VkWriteDescriptorSet> Compiler_DescriptorSet::generate_writes(VkDescriptorSet vk_handle_desc_set)
{
    std::vector<VkWriteDescriptorSet> queued_writes;

    for (const auto& [name, buffer_info] : queued_buffer_writes)
    {
        const auto& binding_info = layout.name_binding_umap.at(name);

        const VkWriteDescriptorSet write {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = vk_handle_desc_set,
            .dstBinding = binding_info.binding,
            .dstArrayElement = 0u,
            .descriptorCount = 1u,
            .descriptorType = binding_info.descriptorType,
            .pImageInfo = nullptr,
            .pBufferInfo = &buffer_info,
            .pTexelBufferView = nullptr,
        };

        queued_writes.push_back(write);
    }

    for (const auto& [name, image_info] : queued_image_writes)
    {
        const auto& binding_info = layout.name_binding_umap.at(name);

        const VkWriteDescriptorSet write {
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .pNext = nullptr,
            .dstSet = vk_handle_desc_set,
            .dstBinding = binding_info.binding,
            .dstArrayElement = 0u,
            .descriptorCount = 1u,
            .descriptorType = binding_info.descriptorType,
            .pImageInfo = &image_info,
            .pBufferInfo = nullptr,
            .pTexelBufferView = nullptr,
        };

        queued_writes.push_back(write);
    }

    return queued_writes;
}

Compiler_DescriptorSet::Compiler_DescriptorSet(VkDescriptorPool _vk_handle_pool, const DescriptorSetLayout& _layout)
    : vk_handle_pool{_vk_handle_pool}
    , layout{_layout}
{}

void Compiler_DescriptorSet::write(std::string name, VkDescriptorBufferInfo&& info)
{
    queued_buffer_writes[name] = std::move(info);
}

void Compiler_DescriptorSet::write(std::string name, VkDescriptorImageInfo&& info)
{
    queued_image_writes[name] = std::move(info);
}

VkDescriptorSet Compiler_DescriptorSet::compile()
{
    if (!sanity_check())
        return {VK_NULL_HANDLE};

    const VkDescriptorSet vk_handle_desc_set = alloc_set(); 

    const std::vector<VkWriteDescriptorSet> queued_writes = generate_writes(vk_handle_desc_set);

    // vkUpdateDescriptorSets

    return vk_handle_desc_set;
}

/*  */

int main()
{
    Compiler_DescriptorSetLayout compiler_layout;

    compiler_layout.register_binding("frame_UBO", 0u, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);
    compiler_layout.register_binding("draw_UBO" , 1u, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1u, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    const DescriptorSetLayout desc_set_layout = compiler_layout.compile();

    Compiler_DescriptorSet compiler_set(VK_NULL_HANDLE, desc_set_layout);

    compiler_set.write("frame_UBO", VkDescriptorBufferInfo{});
    compiler_set.write("draw_UBO" , VkDescriptorBufferInfo{});

    const VkDescriptorSet desc_set = compiler_set.compile();

    return 0;
}