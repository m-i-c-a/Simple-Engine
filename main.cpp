#include <vulkan/vulkan.h>

#include <array>
#include <vector>
#include <string>

constexpr uint32_t FRAMES_IN_FLIGHT = 3;

struct DescriptorSet
{
private:
    VkDescriptorSet m_vk_handle_desc_set;
    std::vector<bool> m_bindings_written;

    // const DescriptorSetLayout* const m_desc_set_layout;
public:
    // explicit DescriptorSet(const DescriptorSetLayout* desc_set_layout);

    void write(const std::string& resource_name, const VkDescriptorBufferInfo& desc_info)
    {

    }
};

#include "ShaderCompiler.hpp"

int main()
{
    // Create any shader resources used
    VkDescriptorPool vk_handle_desc_pool;

    // Pass in used resources to shader via Descriptor*Info structs

    Compiler_ComputeProgram compiler;

    compiler.set_shaders({"shader.comp"});

    compiler.inherit_descriptor_set(0, {VK_NULL_HANDLE});

    compiler.write_descriptor_resource("resource_name_0", {VkDescriptorBufferInfo{}});
    compiler.write_descriptor_resource("resource_name_1", {VkDescriptorBufferInfo{}});

    compiler.write_descriptor_resource("resource_name_1", {VkDescriptorBufferInfo{}});

    // In compile we do all internal hashing and only create new descriptor sets if they are unique
    const Program program = compiler.compile();

    // program.set(0).write("resource_name", VkDescriptorBufferInfo{});


    VkCommandBuffer vk_handle_command_buffer;

    program.bind(vk_handle_command_buffer);

    program.bind_descriptor_sets(vk_handle_command_buffer, 0);





    std::vector<std::string> shader_path_vec;
    // -- reflect pipeline layout
    // -- reflect descriptor set layout
    // -- populate desc_set_layout_hash_array

    // Pipeline State
    // -- create pipeline

    uint32_t local_desc_set_count;
    uint32_t responsibility_index;

    VkDescriptorSetAllocateInfo

}