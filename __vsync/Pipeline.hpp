#include <vulkan/vulkan.h>

#include <vector>
#include <string>

struct Compiler_GraphicsProgram
{
private:
    static std::string m_shader_root_dir;

    std::vector<std::string> m_shader_path_vec;
    std::vector<VkVertexInputBindingDescription> m_vertex_binding_desc_vec;
    std::vector<VkVertexInputAttributeDescription> m_vertex_atrrib_desc_vec;
    VkPrimitiveTopology m_topology {VK_PRIMITIVE_TOPOLOGY_MAX_ENUM};
    std::vector<VkViewport> m_viewport_vec;
    std::vector<VkRect2D> m_scissor_vec;
    VkPolygonMode m_polygon_mode {VK_POLYGON_MODE_MAX_ENUM};
    VkCullModeFlags m_cull_mode {VK_CULL_MODE_FLAG_BITS_MAX_ENUM};
    VkFrontFace m_front_face {VK_FRONT_FACE_MAX_ENUM};
    VkSampleCountFlagBits m_sample_count {VK_SAMPLE_COUNT_FLAG_BITS_MAX_ENUM};
    VkBool32 m_depth_test_enable;
    VkBool32 m_depth_write_enable;
    VkCompareOp m_depth_compare_op;
    std::pair<float, float> m_min_max_depth;
    std::vector<VkPipelineColorBlendAttachmentState> m_color_attachment_blend_state_vec;
    std::vector<VkFormat> m_color_attachment_format_vec;
public:
    static void set_shader_root_dir(const std::string& shader_root_dir);

    void set_shaders(std::vector<std::string>&& shader_path_vec);
    void set_vertex_input_bindings(std::vector<VkVertexInputBindingDescription>&& binding_desc_vec);
    void set_vertex_input_attributes(std::vector<VkVertexInputAttributeDescription>&& attribute_desc_vec);
    void set_topology(VkPrimitiveTopology topology);
    void set_viewport(std::vector<VkViewport>&& viewport_vec);
    void set_scissor(std::vector<VkRect2D>&& scissor_vec);
    void set_polygon_state(VkPolygonMode polygon_mode, VkCullModeFlags cull_mode, VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE);
    void set_multisample_count(VkSampleCountFlagBits sample_count);
    void set_depth_state(VkBool32 test_enable, VkBool32 write_enable, VkCompareOp compare_op, std::pair<float, float> m_min_max_depth);
    void set_color_attachment_blend_state(const std::vector<VkPipelineColorBlendAttachmentState>& state_vec);
    void set_color_attachment_format(const std::vector<VkFormat>& format_vec);

    std::pair<VkPipeline, VkPipelineLayout> compile();
};