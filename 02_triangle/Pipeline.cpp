#include "Pipeline.hpp"
#include "defines.hpp"

#include "vk_core.hpp"

namespace 
{
    VkShaderStageFlagBits get_shader_stage(const std::string& shader_name)
    {
        if (shader_name.ends_with(".vert"))
            return VK_SHADER_STAGE_VERTEX_BIT;
        if (shader_name.ends_with(".geom"))
            return VK_SHADER_STAGE_GEOMETRY_BIT;
        if (shader_name.ends_with(".frag"))
            return VK_SHADER_STAGE_FRAGMENT_BIT;

        return  VK_SHADER_STAGE_FLAG_BITS_MAX_ENUM;
    };


    VkShaderModule create_shader_module(const std::string& shader_name)
    {
        const std::string filepath = shader_name + ".spv";

        FILE* f = fopen(filepath.c_str(), "r");
        ASSERT(f != 0, "Failed to open file %s!\n", filepath.c_str());

        fseek(f, 0, SEEK_END);
        const size_t nbytes_file_size = (size_t)ftell(f);
        rewind(f);

        uint32_t* buffer = (uint32_t*)malloc(nbytes_file_size);
        fread(buffer, nbytes_file_size, 1, f);
        fclose(f);

        const VkShaderModuleCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = nbytes_file_size,
            .pCode = buffer,
        };

        const VkShaderModule shader_module = vk_core::create_shader_module(create_info);

        free(buffer);

        return shader_module;
    };

};

void Compiler_GraphicsProgram::set_shaders(std::vector<std::string>&& shader_path_vec)
{
    m_shader_path_vec = shader_path_vec;
}

void Compiler_GraphicsProgram::set_vertex_input_bindings(std::vector<VkVertexInputBindingDescription>&& binding_desc_vec)
{
    m_vertex_binding_desc_vec = binding_desc_vec;
}

void Compiler_GraphicsProgram::set_vertex_input_attributes(std::vector<VkVertexInputAttributeDescription>&& attribute_desc_vec)
{
    m_vertex_atrrib_desc_vec = attribute_desc_vec;
}

void Compiler_GraphicsProgram::set_viewport(std::vector<VkViewport>&& viewport_vec)
{
    m_viewport_vec = viewport_vec;
}

void Compiler_GraphicsProgram::set_scissor(std::vector<VkRect2D>&& scissor_vec)
{
    m_scissor_vec = scissor_vec;
}

void Compiler_GraphicsProgram::set_polygon_state(VkPolygonMode polygon_mode, VkCullModeFlags cull_mode, VkFrontFace front_face)
{
    m_polygon_mode = polygon_mode;
    m_cull_mode = cull_mode;
    m_front_face = front_face;
}

void Compiler_GraphicsProgram::set_multisample_count(VkSampleCountFlagBits sample_count)
{
    m_sample_count = sample_count;
}


void Compiler_GraphicsProgram::set_depth_state(VkBool32 test_enable, VkBool32 write_enable, VkCompareOp compare_op, std::pair<float, float> min_max_depth)
{
    m_depth_test_enable = test_enable;
    m_depth_write_enable = write_enable;
    m_depth_compare_op = compare_op; 
    m_min_max_depth = min_max_depth;
}

void Compiler_GraphicsProgram::compile()
{
    std::vector<VkPipelineShaderStageCreateInfo> create_info_shader_stage_vec;

    for (const std::string& shader_name : m_shader_path_vec)
    {
        const VkPipelineShaderStageCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .stage = get_shader_stage(shader_name),
            .module = create_shader_module(shader_name),
            .pName = "main",
            .pSpecializationInfo = nullptr,
        };

        create_info_shader_stage_vec.push_back(create_info);
    }

    const VkPipelineVertexInputStateCreateInfo create_info_vertex_input {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .vertexBindingDescriptionCount = static_cast<uint32_t>(m_vertex_binding_desc_vec.size()),
        .pVertexBindingDescriptions = m_vertex_binding_desc_vec.data(),
        .vertexAttributeDescriptionCount = static_cast<uint32_t>(m_vertex_atrrib_desc_vec.size()),
        .pVertexAttributeDescriptions = m_vertex_atrrib_desc_vec.data(),
    };

    const VkPipelineInputAssemblyStateCreateInfo create_info_input_assembly {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .topology = m_topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    const VkPipelineTessellationStateCreateInfo create_info_tesselation {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .patchControlPoints = 0u,
    };

    const VkPipelineViewportStateCreateInfo create_info_viewport {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .viewportCount = static_cast<uint32_t>(m_viewport_vec.size()),
        .pViewports = m_viewport_vec.data(),
        .scissorCount = static_cast<uint32_t>(m_scissor_vec.size()),
        .pScissors = m_scissor_vec.data(),
    };

    const VkPipelineRasterizationStateCreateInfo create_info_rasterization {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = m_polygon_mode,
        .cullMode = m_cull_mode,
        .frontFace = m_front_face,
        .depthBiasEnable = VK_FALSE,
        .depthBiasConstantFactor = 0.0f,
        .depthBiasClamp = 0.0f,
        .depthBiasSlopeFactor = 0.0f,
        .lineWidth = 1.0f,
    };

    const VkPipelineMultisampleStateCreateInfo create_info_multisample_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .rasterizationSamples = m_sample_count,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };

    const VkPipelineDepthStencilStateCreateInfo create_info_depth_stencil {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .depthTestEnable = m_depth_test_enable,
        .depthWriteEnable = m_depth_write_enable,
        .depthCompareOp = m_depth_compare_op,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .front = VK_STENCIL_OP_MAX_ENUM,
        .back = VK_STENCIL_OP_MAX_ENUM,
        .minDepthBounds = m_min_max_depth.first,
        .maxDepthBounds = m_min_max_depth.second,
    };

    const VkPipelineColorBlendStateCreateInfo create_info_blend_state {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .logicOpEnable = ,
        .logicOp = ,
        .attachmentCount = ,
        .pAttachments = ,
        .blendConstants[4] = ,
    };

    const VkGraphicsPipelineCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .stageCount = static_cast<uint32_t>(create_info_shader_stage_vec.size()),
        .pStages = create_info_shader_stage_vec.data(),
        .pVertexInputState = &create_info_vertex_input,
        .pInputAssemblyState = &create_info_input_assembly,
        .pTessellationState = &create_info_tesselation,
        .pViewportState = &create_info_viewport,
        .pRasterizationState = &create_info_rasterization,
        .pMultisampleState = &create_info_multisample_state,
        .pDepthStencilState = &create_info_depth_stencil,
        .pColorBlendState = ,
        .pDynamicState = nullptr,
        .layout = ,
        .renderPass = ,
        .subpass = ,
        .basePipelineHandle = ,
        .basePipelineIndex = ,
    };
}



#if 0
void Compiler_GraphicsProgram::set_shaders(std::vector<std::string>&& shader_name_vec)
{    
    std::vector<VkPipelineShaderStageCreateInfo> shader_stage_vec;

    for (const std::string& shader_name : shader_name_vec)
    {
        const VkPipelineShaderStageCreateInfo create_info {
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .pNext = nullptr,
            .flags = 0x0,
            .stage = get_shader_stage(shader_name),
            .module = create_shader_module(shader_name),
            .pName = "main",
            .pSpecializationInfo = nullptr,
        };

        shader_stage_vec.push_back(create_info);
    }
}
#endif

void Compiler_GraphicsProgram::set_vertex_input()
{
    VkPipelineVertexInputStateCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        // .vertexBindingDescriptionCount = ,
        .pVertexBindingDescriptions = ,
        // .vertexAttributeDescriptionCount = ,
        // .pVertexAttributeDescriptions = ,
    };
}


void Compiler_GraphicsProgram::set_input_assembly(VkPrimitiveTopology topology)
{
    VkPipelineInputAssemblyStateCreateInfo create_info {
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .pNext = nullptr,
        .flags = 0x0,
        .topology = topology,
        .primitiveRestartEnable = VK_FALSE,
    };

    VkPipelineViewportStateCreateInfo v {
    // VkStructureType                       sType;
    // const void*                           pNext;
    // VkPipelineViewportStateCreateFlags    flags;
    // uint32_t                              viewportCount;
    // const VkViewport*                     pViewports;
    // uint32_t                              scissorCount;
    // const VkRect2D*                       pScissors;

    };
}