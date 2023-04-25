#include "VulkanRenderer/Scene/DeferredRenderPass.h"

#include <iostream>

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Math/MathUtils.h"


DeferredRenderPass::DeferredRenderPass()
{
    m_extent = getRendererPointer()->getSwapchainInfo().extent;
    // Clear Color
    m_clearValues.resize(8);
    m_clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[1].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[2].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[3].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[4].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[5].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[6].color = { {0.0f, 0.0f, 0.0f, 1.0f} };
    m_clearValues[7].color = { 1.0f, 0.0f };

    createRenderPass();

    //Secondary Features
    createSecondaryFeatures();

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

    VkExtent2D extent = getRendererPointer()->getSwapchainInfo().extent;
    std::vector<ColorAttachmentInfo> colorAttachmentInfos =
    {
        {"position",            extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT},
        {"normal",              extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT},
        {"albedo",              extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT},
        {"metallicRoughness",   extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT},
        {"emissive",            extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT},
        {"ao",                  extent, VK_FORMAT_R32G32B32A32_SFLOAT, VK_SAMPLE_COUNT_1_BIT},

    };
    createColorAttachments(colorAttachmentInfos);

    m_depthAttacment = Image::Create2DImage(
        getRendererPointer()->getSwapchainInfo().extent,
        getRendererPointer()->getDepthBuffer()->getFormat(),
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT ,
        VMA_MEMORY_USAGE_GPU_ONLY,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        VK_IMAGE_TILING_OPTIMAL,
        VK_SAMPLE_COUNT_1_BIT
    );




    // - Attachments
    std::array<VkAttachmentDescription, 8> attachments{};

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

    // Metallic & roughness
    attachments[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[4].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[4].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[4].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[4].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[4].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[4].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // Emissive
    attachments[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[5].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[5].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[5].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[5].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[5].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[5].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[5].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    // AO
    attachments[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attachments[6].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[6].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[6].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[6].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[6].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[6].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[6].finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    // Depth attachment
    attachments[7].format = depthBufferFormat;
    attachments[7].samples = VK_SAMPLE_COUNT_1_BIT;
    attachments[7].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    attachments[7].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[7].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    attachments[7].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    attachments[7].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    attachments[7].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


    // 三通道
    std::array<VkSubpassDescription, 2> subpassDescriptions{};
    // 第一通道: 写入G-Buffer
    // ----------------------------------------------------------------------------------------
    std::vector<VkAttachmentReference> colorReferences(7);
    colorReferences[0] = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[1] = { 1, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[2] = { 2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[3] = { 3, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[4] = { 4, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[5] = { 5, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    colorReferences[6] = { 6, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };
    VkAttachmentReference depthReference = { 7, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

    subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[0].colorAttachmentCount = colorReferences.size();
    subpassDescriptions[0].pColorAttachments = colorReferences.data();
    subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

    //// 第二通道: 使用G-Buffer处理光照
    //// ----------------------------------------------------------------------------------------

    VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

    VkAttachmentReference inputReferences[6];
    inputReferences[0] = { 1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[1] = { 2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[2] = { 3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[3] = { 4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[4] = { 5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
    inputReferences[5] = { 6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

    uint32_t preserveAttachmentIndex = 1;

    subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpassDescriptions[1].colorAttachmentCount = 1;
    subpassDescriptions[1].pColorAttachments = &colorReference;
    subpassDescriptions[1].pDepthStencilAttachment = &depthReference;
    //使用第一轮中填充的颜色附件作为输入附件
    subpassDescriptions[1].inputAttachmentCount = 6;
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
        { attachments[0], attachments[1], attachments[2],attachments[3],attachments[4],attachments[5] ,attachments[6] ,attachments[7] },
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
            m_colorAttachments[0]->getImageView(),
            m_colorAttachments[1]->getImageView(),
            m_colorAttachments[2]->getImageView(),
            m_colorAttachments[3]->getImageView(),
            m_colorAttachments[4]->getImageView(),
            m_colorAttachments[5]->getImageView(),
            m_depthAttacment->getImageView()
     
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


void DeferredRenderPass::createSecondaryFeatures()
{
    //ShadowMap
    const VkExtent2D swapChainExtent = getRendererPointer()->getSwapchainInfo().extent;
    const VkExtent2D shadowExtent = { 2 * swapChainExtent.height, 2 * swapChainExtent.width };

    m_shadowMap = std::make_shared<ShadowMap>(
        shadowExtent,
        getRendererPointer()->getSwapchainInfo().imageViews.size(),
        getRendererPointer()->getDepthImageInfo().depth_image_format,
        Config::MAX_FRAMES_IN_FLIGHT
        );

    uint32_t finalPassIndex = 1;
    m_skyBox = std::make_shared<SkyBox>(m_renderPass.get(), VK_SAMPLE_COUNT_1_BIT, finalPassIndex);
    m_lightSphere = std::make_shared<LightSphere>(m_renderPass.get(), VK_SAMPLE_COUNT_1_BIT, finalPassIndex);

    //GUI
    m_GUI = std::make_unique<GUI>();

    // IBL
    std::string TextureName = "BRDF_LUT.png";
    TextureToLoadInfo info = { TextureName,"/defaultTextures",VK_FORMAT_R8G8B8A8_SRGB,4 };

    m_BRDFlut = loadTexture(TextureName, std::string(MODEL_DIR) + info.folderName, info.format);
    m_prefilteredIrradiance = std::make_shared<PrefilteredIrradiance>(Config::PREF_IRRADIANCE_DIM);
    m_prefilteredEnvMap = std::make_shared<PrefilteredEnvMap>(Config::PREF_ENV_MAP_DIM);

    getRenderResource()->updateIBLResource(m_BRDFlut, m_prefilteredIrradiance->get(), m_prefilteredEnvMap->get());
}


void DeferredRenderPass::createPipelines()
{
    m_pipelines.resize(PIPELINE_NUM);
    m_descriptorSetLayouts.resize(PIPELINE_NUM);
    m_pipelineLayouts.resize(PIPELINE_NUM);

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

        if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayouts[scene_gbuffer]) != VK_SUCCESS)
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
        std::array<VkPipelineColorBlendAttachmentState, 7> blendAttachmentStates = {
                PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
                PipelineManager::pipelineColorBlendAttachmentState(0xf, VK_FALSE),
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
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayouts[scene_gbuffer];
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pPushConstantRanges = nullptr;
        auto status = vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayouts[scene_gbuffer]);
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
        pipelineInfo.layout = m_pipelineLayouts[scene_gbuffer];
        pipelineInfo.renderPass = m_renderPass.get();
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipelines[scene_gbuffer]);
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

        if (vkCreateDescriptorSetLayout(getRendererPointer()->getDevice(), &layoutInfo, nullptr, &m_descriptorSetLayouts[composition]) != VK_SUCCESS)
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

        VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = PipelineManager::pipelineLayoutCreateInfo(&m_descriptorSetLayouts[composition]);
        vkCreatePipelineLayout(getRendererPointer()->getDevice(), &pipelineLayoutCreateInfo, nullptr, &m_pipelineLayouts[composition]);


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
        pipelineInfo.layout = m_pipelineLayouts[composition];
        pipelineInfo.renderPass = m_renderPass.get();
        pipelineInfo.subpass = 1;       // Important!!
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex = -1;

        auto status = vkCreateGraphicsPipelines(getRendererPointer()->getDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipelines[composition]);

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
    m_shadowMap->updateUBO();
    const glm::mat4 lightSpace = m_shadowMap->getLightSpace();

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
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_meshesUBOAllocationMap[meshIndex][0], &data);
            memcpy(data, &uboData1, sizeof(uboData1));
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_meshesUBOAllocationMap[meshIndex][0]);
        }
    }

    // OnScreen SubPass
    // Normal Infos
    {
        DescriptorTypes::UniformBufferObject::Deferred uboData;
        uboData.cameraPos = glm::vec4(getRenderResource()->m_camera.getCameraPos(), 1.0f);
        uboData.lightsCount = getRenderResource()->m_lightsInfo.size();
        uboData.lightSpace = lightSpace;

        void* data;
        vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_compositionUBOAllocation[0], &data);
        memcpy(data, &uboData, sizeof(uboData) );
        vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_compositionUBOAllocation[0]);
    }
    

    // Light Infos
    {
        DescriptorTypes::UniformBufferObject::LightInfo uboData[Config::LIGHTS_COUNT];

        for (uint32_t i = 0; i < getRenderResource()->m_lightsInfo.size(); i++)
        {
            LightInfo info = getRenderResource()->m_lightsInfo[i];
            uboData[i].pos = glm::vec4(info.pos, 1.0f);
            uboData[i].color = glm::vec4(info.m_color, 1.0f);
            uboData[i].dir = glm::vec4(info.m_targetPos - info.pos, 1.0f);
            uboData[i].radius = 200.0f;
            uboData[i].intensity = info.m_intensity;
            uboData[i].type = (int)info.m_lightType;
        }

        void* data;
        vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_compositionUBOAllocation[1], &data);
        memcpy(data, &uboData, sizeof(uboData[0]) * 10);
        vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_compositionUBOAllocation[1]);
    }
}

void DeferredRenderPass::draw(uint32_t imageIndex, uint32_t currentFrame)
{
    VkCommandBuffer& commandBuffer = getRendererPointer()->getGraphicsCommandBuffer(currentFrame);
    // Resets the command buffer to be able to be recorded.
    vkResetCommandBuffer(commandBuffer, 0);

    // Specifies some details about the usage of this specific command buffer.
    CommandManager::cmdBeginCommandBuffer(commandBuffer, (VkCommandBufferUsageFlagBits)0);

    ////ShadowMap
    m_shadowMap->draw(imageIndex, currentFrame);

    //--------------------------------RenderPass-----------------------------
    VkExtent2D extent = getRendererPointer()->getSwapchainInfo().extent;
    m_renderPass.begin(*m_swapchain_framebuffers[imageIndex], extent, m_clearValues, commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

    // 第一子通道
    // 将场景的组件呈现给G-Buffer附件
    {
        drawPipeline(commandBuffer, m_pipelines[scene_gbuffer], m_pipelineLayouts[scene_gbuffer], getRenderResource()->m_normalModels);
    }

    // 第二子通道
    // 此子通道将使用已在第一子通道中填充的G-Buffer组件作为最终合成的输入附件
    {
        vkCmdNextSubpass(commandBuffer, VK_SUBPASS_CONTENTS_INLINE);

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelines[composition]);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayouts[composition], 0, 1, &m_compositionDescriptorSet.get(), 0, NULL);
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



    // --------------------  onscreen pass -------------------------

    std::vector<size_t> uboSizeInfo = { 
        sizeof(DescriptorTypes::UniformBufferObject::Deferred),
        sizeof(DescriptorTypes::UniformBufferObject::LightInfo) * 10
    };
   
    m_compositionUBO.resize(uboSizeInfo.size());
    m_compositionUBOAllocation.resize(uboSizeInfo.size());

    //Normal
    BufferManager::bufferCreateBuffer(
        getRendererPointer()->getVmaAllocator(),
        uboSizeInfo[0],
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        &m_compositionUBO[0],
        &m_compositionUBOAllocation[0]
    );

    //Lights
    BufferManager::bufferCreateBuffer(
        getRendererPointer()->getVmaAllocator(),
        uboSizeInfo[1],
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU,
        &m_compositionUBO[1],
        &m_compositionUBOAllocation[1]
    );

}

void DeferredRenderPass::createDescriptorSets()
{
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            //-------Pass offscreen -----------
            {
                DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayouts[scene_gbuffer], &m_meshesDescriptorSetMap[meshIndex].get());

                RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];

                std::vector<DescriptorSet::DescriptorSetWriteData> data{
                  { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_meshesUBOMap[meshIndex][0], 0, VK_WHOLE_SIZE},

                  { 1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, renderMeshInfo.ref_material->colorTexture.sampler->getSampler(),                 renderMeshInfo.ref_material->colorTexture.image->getImageView(),                VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                  { 2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, renderMeshInfo.ref_material->metallic_RoughnessTexture.sampler->getSampler(),    renderMeshInfo.ref_material->metallic_RoughnessTexture.image->getImageView(),   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                  { 3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, renderMeshInfo.ref_material->emissiveTexture.sampler->getSampler(),              renderMeshInfo.ref_material->emissiveTexture.image->getImageView(),             VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                  { 4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, renderMeshInfo.ref_material->AOTexture.sampler->getSampler(),                    renderMeshInfo.ref_material->AOTexture.image->getImageView(),                   VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                  { 5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, renderMeshInfo.ref_material->normalTexture.sampler->getSampler(),                renderMeshInfo.ref_material->normalTexture.image->getImageView(),               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                };

                m_meshesDescriptorSetMap[meshIndex].UpdateBindingData(data);
            }
        }
    }

    //-------Pass onscreen -----------
    {
        DescriptorManager::allocDescriptorSet(getRendererPointer()->getDescriptorPool(), m_descriptorSetLayouts[composition], &m_compositionDescriptorSet.get());


        std::vector<DescriptorSet::DescriptorSetWriteData> data{
                 { 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_compositionUBO[0], 0, VK_WHOLE_SIZE},
                 { 1, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, m_compositionUBO[1], 0, VK_WHOLE_SIZE},

                 { 2, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_colorAttachments[0]->getImageView(),  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL},
                 { 3, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_colorAttachments[1]->getImageView(),  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                 { 4, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_colorAttachments[2]->getImageView(),  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                 { 5, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_colorAttachments[3]->getImageView(),  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                 { 6, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_colorAttachments[4]->getImageView(),  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                 { 7, VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, m_colorAttachments[5]->getImageView(),  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },

                 { 8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, getRenderResource()->m_IBLResource.irradiance.sampler->getSampler(),             getRenderResource()->m_IBLResource.irradiance.image->getImageView(),            VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                 { 9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, getRenderResource()->m_IBLResource.brdfLUT.sampler->getSampler(),                getRenderResource()->m_IBLResource.brdfLUT.image->getImageView(),               VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },
                 { 10,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, getRenderResource()->m_IBLResource.prefiltered_Env.sampler->getSampler(),        getRenderResource()->m_IBLResource.prefiltered_Env.image->getImageView(),       VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL },

                 { 11,VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, m_shadowMap->getSampler(),                                                       m_shadowMap->getImage()->getImageView(),                                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL }
        };
        m_compositionDescriptorSet.UpdateBindingData(data);

    }
}


void DeferredRenderPass::destroy()
{
    for (auto& framebuffer : m_swapchain_framebuffers)
        vkDestroyFramebuffer(getRendererPointer()->getDevice(), *framebuffer, nullptr);

    for (auto& uboInfo : m_meshesUBOMap)
    {
        for (uint32_t i = 0; i < uboInfo.second.size(); ++i)
        {
            vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), uboInfo.second[i], m_meshesUBOAllocationMap[uboInfo.first][i]);
        }
    }

    for(int i = 0 ; i < m_compositionUBO.size(); i++)
        vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(),m_compositionUBO[i], m_compositionUBOAllocation[i]);


    for (auto attachment : m_colorAttachments)
        attachment->destroy();

    m_depthAttacment->destroy();

    m_BRDFlut.image->destroy();
    m_BRDFlut.sampler->destroy();

    m_prefilteredEnvMap->destroy();
    m_prefilteredIrradiance->destroy();

    // ImGui
    m_GUI->destroy();
    m_shadowMap->destroy();
    m_skyBox->destroy();
    m_lightSphere->destroy();


    for (int i = 0; i < PIPELINE_NUM; i++)
    {
        vkDestroyDescriptorSetLayout(getRendererPointer()->getDevice(), m_descriptorSetLayouts[i], nullptr);
        vkDestroyPipeline(getRendererPointer()->getDevice(), m_pipelines[i], nullptr);
        vkDestroyPipelineLayout(getRendererPointer()->getDevice(), m_pipelineLayouts[i], nullptr);
    }
  
    m_renderPass.destroy();
}
