#include "VulkanRenderer/Scene/DeferredRenderPass.h"

#include <iostream>

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Math/MathUtils.h"
#include "VulkanRenderer/Shader/ShaderManager.h"
#include "VulkanRenderer/Pipeline/PipelineManager.h"

DeferredRenderPass::DeferredRenderPass()
{
    m_extent = getRendererPointer()->getSwapchainInfo().extent;
    // Clear Color
    m_clearValues.resize(5);
    m_clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[3].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[4].color = { 1.0f, 0.0f };

    //GUI
    m_GUI = std::make_unique<GUI>();

    createRenderPass();

    uint32_t finalPassIndex = 1;
    m_skyBox = std::make_shared<SkyBox>(m_renderPass.get(), VK_SAMPLE_COUNT_1_BIT, finalPassIndex);
    m_lightSphere = std::make_shared<LightSphere>(m_renderPass.get(), VK_SAMPLE_COUNT_1_BIT, finalPassIndex);

    createPipelines();
    
    createUBOs();
    createDescriptorSets();

    createSwapchainFramebuffers();
}


DeferredRenderPass::~DeferredRenderPass() {}


void DeferredRenderPass::createRenderPass()
{
    VkFormat format = getRendererPointer()->getSwapchainInfo().image_format;
    VkFormat depthBufferFormat = getRendererPointer()->getDepthImageInfo().depth_image_format;
    VkSampleCountFlagBits msaaSamplesCount = getRendererPointer()->getMSAAInfo().msaa_sampleCount;


    m_attachmentPos = std::make_shared<NormalTexture>("position");
    m_attachmentPos->getExtent() = getRendererPointer()->getSwapchainInfo().extent;
    m_attachmentPos->getFormat() = VK_FORMAT_R32G32B32A32_SFLOAT;

    BufferManager::bufferCreateOffscreenResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        m_attachmentPos->getExtent(),
        m_attachmentPos->getFormat(),
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        1,
        1,
        0,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_VIEW_TYPE_2D,
        &m_attachmentPos->getAllocation(),
        m_attachmentPos
    );

    m_attachmentNormal = std::make_shared<NormalTexture>("normal");
    m_attachmentNormal->getExtent() = getRendererPointer()->getSwapchainInfo().extent;
    m_attachmentNormal->getFormat() = VK_FORMAT_R32G32B32A32_SFLOAT;

    BufferManager::bufferCreateOffscreenResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        m_attachmentNormal->getExtent(),
        m_attachmentNormal->getFormat(),
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        1,
        1,
        0,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_VIEW_TYPE_2D,
        &m_attachmentNormal->getAllocation(),
        m_attachmentNormal
    );

    m_attachmentAlbedo = std::make_shared<NormalTexture>("albedo");
    m_attachmentAlbedo->getExtent() = getRendererPointer()->getSwapchainInfo().extent;
    m_attachmentAlbedo->getFormat() = VK_FORMAT_R32G32B32A32_SFLOAT;

    BufferManager::bufferCreateOffscreenResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        m_attachmentAlbedo->getExtent(),
        m_attachmentAlbedo->getFormat(),
        VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
        1,
        1,
        0,
        VK_SAMPLE_COUNT_1_BIT,
        VK_IMAGE_VIEW_TYPE_2D,
        &m_attachmentAlbedo->getAllocation(),
        m_attachmentAlbedo
    );

    m_attachmentDepth = std::make_shared<NormalTexture>("depth");
    m_attachmentDepth->getExtent() = getRendererPointer()->getSwapchainInfo().extent;
    m_attachmentDepth->getFormat() = getRendererPointer()->getDepthBuffer()->getFormat();

    BufferManager::bufferCreateDepthResources(
        getRendererPointer()->getDevice(),
        getRendererPointer()->getVmaAllocator(),
        getRendererPointer()->getGraphicsQueue(),
        getRendererPointer()->getCommandPool(),
        m_attachmentDepth->getExtent(),
        m_attachmentDepth->getFormat(),
        VK_SAMPLE_COUNT_1_BIT,
        &m_attachmentDepth->getImage(),
        &m_attachmentDepth->getAllocation(),
        &m_attachmentDepth->getImageView()
    );


    // - Attachments
    std::array<VkAttachmentDescription, 5> attachments{};

    // 场景颜色附件
    attachments[0].format = format;
    attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[0].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // G-Buffer 附件
    // Position
    attachments[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[1].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Normals
    attachments[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[2].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[2].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[2].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[2].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[2].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[2].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Albedo
    attachments[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[3].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[3].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[3].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[3].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[3].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[3].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Depth attachment
    attachments[4].format = depthBufferFormat;
    attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[4].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    // 三通道
    std::array<VkSubpassDescription, 2> subpassDescriptions{};
    // 第一通道: 写入G-Buffer
    // ----------------------------------------------------------------------------------------
    std::vector<VkAttachmentReference> colorReferences(4);
    colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthReference = { 4, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[0].colorAttachmentCount = colorReferences.size();
    subpassDescriptions[0].pColorAttachments = colorReferences.data();
    subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

    //// 第二通道: 使用G-Buffer处理光照
    //// ----------------------------------------------------------------------------------------

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkAttachmentReference inputReferences[3];
    inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    uint32_t preserveAttachmentIndex = 1;

    subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[1].colorAttachmentCount = 1;
    subpassDescriptions[1].pColorAttachments = &colorReference;
    subpassDescriptions[1].pDepthStencilAttachment = &depthReference;
    //使用第一轮中填充的颜色附件作为输入附件
    subpassDescriptions[1].inputAttachmentCount = 3;
    subpassDescriptions[1].pInputAttachments = inputReferences;

    //// 第三通道: 前向透明
    //    // ----------------------------------------------------------------------------------------
    //colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    //inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    //subpassDescriptions[2].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    //subpassDescriptions[2].colorAttachmentCount = 1;
    //subpassDescriptions[2].pColorAttachments = &colorReference;
    //subpassDescriptions[2].pDepthStencilAttachment = &depthReference;
    //// 使用第一轮中填充的颜色 / 深度附件作为输入附件
    //subpassDescriptions[2].inputAttachmentCount = 1;
    //subpassDescriptions[2].pInputAttachments = inputReferences;


    // 子通道布局转换的依赖关系
    std::array<VkSubpassDependency, 3> dependencies;

    dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[0].dstSubpass = 0;
    dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    // This dependency transitions the input attachment from color attachment to shader read
        //这个依赖关系将输入附件从颜色附件转换到着色器读取
    dependencies[1].srcSubpass = 0;
    dependencies[1].dstSubpass = 1;
    dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    dependencies[2].srcSubpass = 0;
    dependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
    dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    dependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //// This dependency transitions the input attachment from color attachment to shader read
    ////这个依赖关系将输入附件从颜色附件转换到着色器读取
    //dependencies[1].srcSubpass = 0;
    //dependencies[1].dstSubpass = 1;
    //dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    //dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //dependencies[2].srcSubpass = 1;
    //dependencies[2].dstSubpass = 2;
    //dependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[2].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    //dependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[2].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
    //dependencies[2].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    //dependencies[3].srcSubpass = 0;
    //dependencies[3].dstSubpass = VK_SUBPASS_EXTERNAL;
    //dependencies[3].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    //dependencies[3].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
    //dependencies[3].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    //dependencies[3].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
    //dependencies[3].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = static_cast<uint32_t>(subpassDescriptions.size());
    renderPassInfo.pSubpasses = subpassDescriptions.data();
    renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
    renderPassInfo.pDependencies = dependencies.data();

    m_renderPass = RenderPass(
        { attachments[0], attachments[1], attachments[2],attachments[3],attachments[4]},
        { subpassDescriptions[0],subpassDescriptions[1]},
        { dependencies[0],dependencies[1],dependencies[2]}
    );
}

void DeferredRenderPass::createSwapchainFramebuffers()
{
    m_swapchain_framebuffers.resize(getRendererPointer()->getSwapchainInfo().imageViews.size());

    // create frame buffer for every imageview
    for (size_t i = 0; i < m_swapchain_framebuffers.size(); i++)
    {
        m_swapchain_framebuffers[i] = new VkFramebuffer;

        std::vector<VkImageView> attachments = {
            *getRendererPointer()->getSwapchainInfo().imageViews[i],
            m_attachmentPos->getImageView(),
            m_attachmentNormal->getImageView(),
            m_attachmentAlbedo->getImageView(),
            m_attachmentDepth->getImageView()
     
        };

        FramebufferManager::createFramebuffer(
            getRendererPointer()->getDevice(),
            m_renderPass.get(),
            attachments,
            m_extent.width,
            m_extent.height,
            1,
            m_swapchain_framebuffers[i]
        );
    }
}


void DeferredRenderPass::createUniformBuffer(const std::shared_ptr<Model> modelPtr, std::vector<size_t>& uboSizeInfos)
{
    for (uint32_t meshIndex : modelPtr->getMeshIndices())
    {
        // create UBO PerMesh
        m_ubosMap[meshIndex].resize(uboSizeInfos.size());
        m_uboAllocationsMap[meshIndex].resize(uboSizeInfos.size());

        for (uint32_t i = 0; i < uboSizeInfos.size(); ++i)
        {
            BufferManager::bufferCreateBuffer(
                getRendererPointer()->getVmaAllocator(),
                uboSizeInfos[i],
                VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
                VMA_MEMORY_USAGE_CPU_TO_GPU,
                &m_ubosMap[meshIndex][i],
                &m_uboAllocationsMap[meshIndex][i]
            );
        }
    }
}


void DeferredRenderPass::createPipelines()
{
    //-------------------------------- Pipeline OffScreen --------------------------------------
    {
        const std::vector<DescriptorInfo>& descriptorInfo = GRAPHICS_PIPELINE::DEFERRED_OFF::DESCRIPTORS_INFO;

        std::vector<VkDescriptorSetLayoutBinding> bindings(descriptorInfo.size());
        for (uint32_t i = 0; i < descriptorInfo.size(); i++)
        {
            bindings[i].binding = descriptorInfo[i].bindingNumber;
            bindings[i].descriptorType = descriptorInfo[i].descriptorType;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = descriptorInfo[i].shaderStage;
            bindings[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout_Offscreen) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");


        // -------------------Shader Modules--------------------
        const std::vector<ShaderInfo>& shaderInfos = { {shaderType::VERTEX, "deferred_off"}, {shaderType::FRAGMENT, "deferred_off"} };

        std::vector<VkShaderModule> shaderModules(shaderInfos.size());
        std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos(shaderInfos.size());
        for (uint32_t i = 0; i < shaderInfos.size(); i++)
        {
            PipelineManager::createShaderModule(shaderInfos[i], shaderModules[i]);
            PipelineManager::createShaderStageInfo(shaderModules[i], shaderInfos[i].type, shaderStagesInfos[i]);
        }


        // Dynamic states
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = PipelineManager::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());

        // Viewport
        VkViewport viewport{};
        PipelineManager::createViewport(viewport, m_extent);
        // Scissor
        VkRect2D scissor{};
        PipelineManager::createScissor(scissor, m_extent);
        // Viewport state info
        VkPipelineViewportStateCreateInfo viewportState = PipelineManager::pipelineViewportStateCreateInfo(1, 1, 0);
        // Vertex input(attributes)
        std::vector<VkVertexInputAttributeDescription> attribDescription = Attributes::DEFERRED_OFF::getAttributeDescriptions();
        VkVertexInputBindingDescription bindingDescription = Attributes::DEFERRED_OFF::getBindingDescription();
        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        PipelineManager::createVertexShaderInputInfo(bindingDescription, attribDescription, vertexInputInfo);
        // Input assembly
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = PipelineManager::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        // Rasterizer
        VkPipelineRasterizationStateCreateInfo rasterizationState = PipelineManager::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        // Multisampling 
        VkPipelineMultisampleStateCreateInfo multisampleState = PipelineManager::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        // Color blending(attachment)
        std::array<VkPipelineColorBlendAttachmentState, 4> blendAttachmentStates = {
                PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE)
        };
        // Color blending(global)
        VkPipelineColorBlendStateCreateInfo colorBlendState = PipelineManager::pipelineColorBlendStateCreateInfo(blendAttachmentStates.size(), blendAttachmentStates.data());

        // Pipeline layout
        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout_Offscreen;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        auto status = vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout_Offscreen);
        if (status != VK_SUCCESS)
            throw std::runtime_error("Failed to create pipeline layout!");

        // Depth and stencil
        VkPipelineDepthStencilStateCreateInfo depthStencilState = PipelineManager::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);


        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderInfos.size();
        pipelineInfo.pStages = shaderStagesInfos.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizationState;
        pipelineInfo.pMultisampleState = &multisampleState;
        pipelineInfo.pDepthStencilState = &depthStencilState;
        pipelineInfo.pColorBlendState = &colorBlendState;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.pTessellationState = nullptr;
        pipelineInfo.pNext = nullptr;
        pipelineInfo.layout = m_pipelineLayout_Offscreen;
        pipelineInfo.renderPass = m_renderPass.get();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline_Offscreen);
        if (status != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline!");

        for (auto& shaderModule : shaderModules)
        {
            ShaderManager::destroyShaderModule(shaderModule);
        }
    }

    //-------------------------------- Pipeline OffScreen --------------------------------------
    {
        const std::vector<DescriptorInfo>& descriptorInfo = GRAPHICS_PIPELINE::DEFERRED_ON::DESCRIPTORS_INFO;

        std::vector<VkDescriptorSetLayoutBinding> bindings(descriptorInfo.size());
        for (uint32_t i = 0; i < descriptorInfo.size(); i++)
        {
            bindings[i].binding = descriptorInfo[i].bindingNumber;
            bindings[i].descriptorType = descriptorInfo[i].descriptorType;
            bindings[i].descriptorCount = 1;
            bindings[i].stageFlags = descriptorInfo[i].shaderStage;
            bindings[i].pImmutableSamplers = nullptr;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayout_Onscreen) != VK_SUCCESS)
            throw std::runtime_error("Failed to create descriptor set layout!");


        // -------------------Shader Modules--------------------
        const std::vector<ShaderInfo>& shaderInfos = { {shaderType::VERTEX, "deferred_on"}, {shaderType::FRAGMENT, "deferred_on"} };

        std::vector<VkShaderModule> shaderModules(shaderInfos.size());
        std::vector<VkPipelineShaderStageCreateInfo> shaderStagesInfos(shaderInfos.size());
        for (uint32_t i = 0; i < shaderInfos.size(); i++)
        {
            PipelineManager::createShaderModule(shaderInfos[i], shaderModules[i]);
            PipelineManager::createShaderStageInfo(shaderModules[i], shaderInfos[i].type, shaderStagesInfos[i]);
        }

        // -Vertex input(attributes)
        // Gets the binding and descriptions of the triangle's vertices and vertex attributes(one array containing both).
        VkPipelineVertexInputStateCreateInfo emptyInputState{};
        emptyInputState.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        // Pipeline
        VkPipelineInputAssemblyStateCreateInfo inputAssemblyState = PipelineManager::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, 0, VK_FALSE);
        VkPipelineRasterizationStateCreateInfo rasterizationState = PipelineManager::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_COUNTER_CLOCKWISE, 0);
        VkPipelineColorBlendAttachmentState blendAttachmentState = PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE);
        VkPipelineColorBlendStateCreateInfo colorBlendState = PipelineManager::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);
        VkPipelineDepthStencilStateCreateInfo depthStencilState = PipelineManager::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_FALSE, VK_COMPARE_OP_LESS_OR_EQUAL);
        VkPipelineViewportStateCreateInfo viewportState = PipelineManager::pipelineViewportStateCreateInfo(1, 1, 0);
        VkPipelineMultisampleStateCreateInfo multisampleState = PipelineManager::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT, 0);
        std::vector<VkDynamicState> dynamicStateEnables = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
        VkPipelineDynamicStateCreateInfo dynamicState = PipelineManager::pipelineDynamicStateCreateInfo(dynamicStateEnables.data(), dynamicStateEnables.size());

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = PipelineManager::pipelineLayoutCreateInfo(&m_descriptorSetLayout_Onscreen);
        vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayout_Onscreen);


        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = shaderInfos.size();
        pipelineInfo.pStages = shaderStagesInfos.data();

        // Reference to all the structures describing the fixed-function stage
        pipelineInfo.pVertexInputState = &emptyInputState;
        pipelineInfo.pInputAssemblyState = &inputAssemblyState;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizationState;
        pipelineInfo.pMultisampleState = &multisampleState;
        pipelineInfo.pDepthStencilState = &depthStencilState;
        pipelineInfo.pColorBlendState = &colorBlendState;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout_Onscreen;
        pipelineInfo.renderPass = m_renderPass.get();
        pipelineInfo.subpass = 1;       // Important!!
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        auto status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline_Onscreen);

        if (status != VK_SUCCESS)
            throw std::runtime_error("Failed to create graphics pipeline!");

        for (auto& shaderModule : shaderModules)
        {
            ShaderManager::destroyShaderModule(shaderModule);
        }
    }
}


void DeferredRenderPass::updateUBO(
    const VkExtent2D& extent,
    const uint32_t& currentFrame
) {
 /*   m_shadowMap->updateUBO();
    const glm::mat4 lightSpace1 = m_shadowMap->getLightSpace();*/

    m_lightSphere->updateUBO();
    m_skyBox->updateUBO();

    // DeferredRenderPass
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            // update normal UBO 
            DescriptorTypes::UniformBufferObject::MVP  uboData1;
            uboData1.model = ptr->getModelMatrix();
            uboData1.view = getRenderResource()->m_camera.getViewMatrix();
            uboData1.proj = getRenderResource()->m_camera.getProjectionMatrix();
    
            void* data;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0], &data);
            memcpy(data, &uboData1, sizeof(uboData1));
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0]);
        }
    }

    // OnScreen SubPass
    DescriptorTypes::UniformBufferObject::LightInfo uboData2[Config::LIGHTS_COUNT];

    for (uint32_t i = 0; i < getRenderResource()->m_lightsInfo.size(); i++)
    {
        LightInfo info = getRenderResource()->m_lightsInfo[i];
        uboData2[i].pos = glm::vec4(info.pos, 1.0f);
        uboData2[i].color = glm::vec4(info.m_color, 1.0f);
        uboData2[i].dir = glm::vec4(info.m_targetPos - info.pos, 1.0f);
        uboData2[i].intensity = info.m_intensity;
        uboData2[i].type = (int)info.m_lightType;
    }

    void* data;
    vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_screenUBOAllocation, &data);
    memcpy(data, &uboData2, sizeof(uboData2[0]) * 10);
    vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_screenUBOAllocation);
}

void DeferredRenderPass::draw(uint32_t imageIndex, uint32_t currentFrame)
{
    VkCommandBuffer& commandBuffer = getRendererPointer()->getGraphicsCommandBuffer(currentFrame);
    // Resets the command buffer to be able to be recorded.
    vkResetCommandBuffer(commandBuffer, 0);

    // Specifies some details about the usage of this specific command buffer.
    CommandManager::cmdBeginCommandBuffer(commandBuffer, (VkCommandBufferUsageFlagBits)0);

    ////ShadowMap
    //m_shadowMap->draw(imageIndex, currentFrame);

    //--------------------------------RenderPass-----------------------------
    VkExtent2D extent = getRendererPointer()->getSwapchainInfo().extent;
    m_renderPass.begin(*m_swapchain_framebuffers[imageIndex], extent, m_clearValues, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    // 第一子通道
    // 将场景的组件呈现给G-Buffer附件
    {
        drawPipeline(commandBuffer, m_pipeline_Offscreen, m_pipelineLayout_Offscreen, getRenderResource()->m_normalModels);
    }

    // 第二子通道
    // 此子通道将使用已在第一子通道中填充的G-Buffer组件作为最终合成的输入附件
    {
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline_Onscreen);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout_Onscreen, 0, 1, &m_screenDescriptorSet, 0, NULL);
        vkCmdDraw(commandBuffer, 3, 1, 0, 0);
    
        m_lightSphere->draw(commandBuffer);
        m_skyBox->draw(commandBuffer);
    }

    m_renderPass.end(commandBuffer);

    //GUI
    m_GUI->draw(currentFrame, imageIndex);

    vkEndCommandBuffer(commandBuffer);
}

void DeferredRenderPass::createUBOs()
{
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        std::vector<size_t> uboSizeInfos = {
               sizeof(DescriptorTypes::UniformBufferObject::MVP)
        };
        createUniformBuffer(ptr, uboSizeInfos);
    }


    std::vector<size_t> uboSizeInfos = {sizeof(DescriptorTypes::UniformBufferObject::LightInfo) * 10};

    BufferManager::bufferCreateBuffer(
        getRendererPointer()->getVmaAllocator(),
        uboSizeInfos[0],
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        &m_screenUBO,
        &m_screenUBOAllocation
    );

}

void DeferredRenderPass::createDescriptorSets()
{
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            m_descriptorSetsMap[meshIndex].resize(1);

            //-------Pass offscreen -----------
            {
                DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayout_Offscreen, &m_descriptorSetsMap[meshIndex][0]);

                RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

                VkDescriptorBufferInfo uniformBufferMVP = DescriptorManager::descriptorBufferInfo(m_ubosMap[meshIndex][0]);
                VkDescriptorImageInfo texDescriptorColor = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->colorTexture->getSampler(), renderMeshInfo.ref_material->colorTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                VkDescriptorImageInfo texDescriptorNormal = DescriptorManager::descriptorImageInfo(renderMeshInfo.ref_material->normalTexture->getSampler(), renderMeshInfo.ref_material->normalTexture->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

                std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
                    // Binding 0: MVP
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBufferMVP),
                    // Binding 1: Position texture target
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1, &texDescriptorColor),
                    // Binding 2: Normals texture target
                    DescriptorManager::writeDescriptorSet(m_descriptorSetsMap[meshIndex][0], VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 2, &texDescriptorNormal),
                };
                vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
            }
        }
    }

    //-------Pass onscreen -----------
    {
        DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayout_Onscreen, &m_screenDescriptorSet);

        VkDescriptorBufferInfo uniformBufferLights = DescriptorManager::descriptorBufferInfo(m_screenUBO);
        VkDescriptorImageInfo texDescriptorPosition = DescriptorManager::descriptorImageInfo(VK_NULL_HANDLE, m_attachmentPos->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkDescriptorImageInfo texDescriptorNormal = DescriptorManager::descriptorImageInfo(VK_NULL_HANDLE, m_attachmentNormal->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        VkDescriptorImageInfo texDescriptorAlbedo = DescriptorManager::descriptorImageInfo(VK_NULL_HANDLE, m_attachmentAlbedo->getImageView(), VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);


        std::vector<VkWriteDescriptorSet> writeDescriptorSets = {
            // Binding 0: Lights
            DescriptorManager::writeDescriptorSet(m_screenDescriptorSet, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 0, &uniformBufferLights),
            // Binding 1: Position texture target
            DescriptorManager::writeDescriptorSet(m_screenDescriptorSet, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1, &texDescriptorPosition),
            // Binding 2: Normals texture target
            DescriptorManager::writeDescriptorSet(m_screenDescriptorSet, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 2, &texDescriptorNormal),
            // Binding 3: Albedo texture target
            DescriptorManager::writeDescriptorSet(m_screenDescriptorSet, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 3, &texDescriptorAlbedo)
        };

        vkUpdateDescriptorSets(getRendererPointer()->getDevice(), static_cast<uint32_t>(writeDescriptorSets.size()), writeDescriptorSets.data(), 0, nullptr);
    }
}


const RenderPass& DeferredRenderPass::getRenderPass() const
{
    return m_renderPass;
}


void DeferredRenderPass::destroy()
{
    for (auto& framebuffer : m_swapchain_framebuffers)
        vkDestroyFramebuffer(getRendererPointer()->getDevice(), *framebuffer, nullptr);

    for (auto& uboInfo : m_ubosMap)
    {
        for (uint32_t i = 0; i < uboInfo.second.size(); ++i)
        {
            vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), uboInfo.second[i], m_uboAllocationsMap[uboInfo.first][i]);
        }
    }
    vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(),m_screenUBO, m_screenUBOAllocation);


    m_attachmentAlbedo->destroy();
    m_attachmentDepth->destroy();
    m_attachmentNormal->destroy();
    m_attachmentPos->destroy();

    // ImGui
    m_GUI->destroy();
    m_skyBox->destroy();
    m_lightSphere->destroy();

    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayout_Offscreen, nullptr);
    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipeline_Offscreen, nullptr);
    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayout_Offscreen, nullptr);

    vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayout_Onscreen, nullptr);
    vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipeline_Onscreen, nullptr);
    vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayout_Onscreen, nullptr);



    m_renderPass.destroy();
}


void DeferredRenderPass::drawPipeline(const VkCommandBuffer& commandBuffer, const VkPipeline& pipeline, const VkPipelineLayout& pipelineLayout, std::vector<std::shared_ptr<Model>> models)
{

    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

    // Set Dynamic States
    VkViewport viewport{ 0.0f, 0.0f, m_extent.width,m_extent.height, 0.0f, 1.0f };
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{ {0,0}, {m_extent.width,m_extent.height} };
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    for (auto& ptr : models)
    {
        if (ptr->isHidden() == false)
        {
            for (uint32_t meshIndex : ptr->getMeshIndices())
            {
                RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

                std::vector<VkDeviceSize> offsets = { 0 };
                vkCmdBindVertexBuffers(commandBuffer, 0, 1, renderMeshInfo.ref_mesh->vertexBuffer, offsets.data());
                vkCmdBindIndexBuffer(commandBuffer, *renderMeshInfo.ref_mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

                const std::vector<VkDescriptorSet> sets = { getMeshDescriptorSet(meshIndex) };
                vkCmdBindDescriptorSets(
                    commandBuffer,
                    VK_PIPELINE_BIND_POINT_GRAPHICS,
                    pipelineLayout,
                    0,
                    sets.size(), sets.data(),
                    0, {}
                );

                vkCmdDrawIndexed(commandBuffer, renderMeshInfo.ref_mesh->meshIndexCount, 1, 0, 0, 0);
            }
        }
    }
}
