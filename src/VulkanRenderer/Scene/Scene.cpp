#include "VulkanRenderer/Scene/Scene.h"

#include <thread>
#include <iostream>

#include "VulkanRenderer/Renderer.h"


Scene::Scene() {}

Scene::Scene(
    const VkFormat& format,
    const VkExtent2D& extent,
    const VkSampleCountFlagBits& msaaSamplesCount,
    const VkFormat& depthBufferFormat,
    const std::vector<ModelInfo>& modelsToLoadInfo,
    // Parameters needed for the computations.
    const QueueFamilyIndices& queueFamilyIndices,
    VkDescriptorPool& descriptorPoolForComputations,
    VmaAllocator& vmaAllocator
) : m_directionalLightIndex(-1), m_vmaAllocator(vmaAllocator)
{
    loadModels(modelsToLoadInfo);

    createRenderPass(format, msaaSamplesCount, depthBufferFormat);

    createPipelines(format, extent, msaaSamplesCount);

    initComputations(queueFamilyIndices,descriptorPoolForComputations);
}

void Scene::initComputations(const QueueFamilyIndices& queueFamilyIndices,VkDescriptorPool& descriptorPoolForComputations) 
{
    m_BRDFcomp = Computation(
        "BRDF",
        sizeof(float),
        2 * sizeof(float) * Config::BRDF_HEIGHT * Config::BRDF_WIDTH,
        queueFamilyIndices,
        descriptorPoolForComputations,
        COMPUTE_PIPELINE::BRDF::BUFFERS_INFO
    );
}

Scene::~Scene() {}


void Scene::createRenderPass(
    const VkFormat& format,
    const VkSampleCountFlagBits& msaaSamplesCount,
    const VkFormat& depthBufferFormat
){
    // - Attachments
    
    // Color Attachment
    VkAttachmentDescription colorAttachment{};
    AttachmentUtils::createAttachmentDescription(
        format,
        msaaSamplesCount,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorAttachment
    );


    // Depth Attachment
    VkAttachmentDescription depthAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        depthBufferFormat,
        msaaSamplesCount,
        VK_ATTACHMENT_LOAD_OP_CLEAR,
        // We don't care about storing the depth data, because it will not be used after drawing has finished.
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        // Just like the color buffer, we don't care about the previous depth contents.
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        depthAttachment
    );


    //Color Resolve Attachment (needed by MSAA)
    VkAttachmentDescription colorResolveAttachment{};
    AttachmentUtils::createAttachmentDescriptionWithStencil(
        format,
        VK_SAMPLE_COUNT_1_BIT,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_STORE,
        VK_ATTACHMENT_LOAD_OP_DONT_CARE,
        VK_ATTACHMENT_STORE_OP_DONT_CARE,
        VK_IMAGE_LAYOUT_UNDEFINED,
        // Here is not 'VK_IMAGE_LAYOUT_PRESENT_SRC_KHR'
        // because the GUI will be the last and the one to present.
        VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        colorResolveAttachment
    );


    // - Attachment References
    VkAttachmentReference colorAttachmentRef{};
    AttachmentUtils::createAttachmentReference(0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorAttachmentRef);

    VkAttachmentReference depthAttachmentRef{};
    AttachmentUtils::createAttachmentReference(1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, depthAttachmentRef);

    VkAttachmentReference colorResolveAttachmentRef{};
    AttachmentUtils::createAttachmentReference(2, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, colorResolveAttachmentRef);



    // - Subpasses
    VkSubpassDescription subPassDescript{};
    SubPassUtils::createSubPassDescription(
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        &colorAttachmentRef,
        &depthAttachmentRef,
        &colorResolveAttachmentRef,
        subPassDescript
    );



    // - Subpass dependices
    VkSubpassDependency dependency{};
    SubPassUtils::createSubPassDependency(
        // -Source parameters.
        // VK_SUBPASS_EXTERNAL means anything outside of a given render pass scope. 
        // When used for srcSubpass it specifies anything that happened before the render pass. 
        VK_SUBPASS_EXTERNAL,
        (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT),
        0,
        0,
        (VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT),
        (VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT),
        (VkDependencyFlagBits) 0,
        dependency
    );



    m_renderPass = RenderPass(
        { colorAttachment, depthAttachment, colorResolveAttachment },
        { subPassDescript },
        { dependency }
    );
}


void Scene::createPipelines(
    const VkFormat& format,
    const VkExtent2D& extent,
    const VkSampleCountFlagBits& msaaSamplesCount
) {
    m_graphicsPipelineSkybox = Graphics(
        GraphicsPipelineType::SKYBOX,
        extent,
        m_renderPass,
        { {shaderType::VERTEX, "skybox"}, {shaderType::FRAGMENT, "skybox"} },
        msaaSamplesCount,
        Attributes::SKYBOX::getBindingDescription(),
        Attributes::SKYBOX::getAttributeDescriptions(),
        { getRenderResource()->m_skyboxIndex },
        GRAPHICS_PIPELINE::SKYBOX::DESCRIPTORS_INFO,
        {}
    );

    m_graphicsPipelinePBR = Graphics(
        GraphicsPipelineType::PBR,
        extent,
        m_renderPass,
        { {shaderType::VERTEX, "scene"}, {shaderType::FRAGMENT, "scene"} },
        msaaSamplesCount,
        Attributes::PBR::getBindingDescription(),
        Attributes::PBR::getAttributeDescriptions(),
        //Models asssocciated with this graphics pipeline.
        getRenderResource()->m_objectModelIndices,
        GRAPHICS_PIPELINE::PBR::DESCRIPTORS_INFO,
        {}
    );

    m_graphicsPipelineLight = Graphics(
        GraphicsPipelineType::LIGHT,
        extent,
        m_renderPass,
        { {shaderType::VERTEX, "light"},{shaderType::FRAGMENT,"light"} },
        msaaSamplesCount,
        Attributes::LIGHT::getBindingDescription(),
        Attributes::LIGHT::getAttributeDescriptions(),
        // Models assocciated with this graphics pipeline.
        getRenderResource()->m_lightModelIndices,
        GRAPHICS_PIPELINE::LIGHT::DESCRIPTORS_INFO,
        {}
    );
}


void Scene::loadModels(const std::vector<ModelInfo>& modelsToLoadInfo)
{
    std::vector<std::thread> threads;

    const size_t maxThreadsCount = std::thread::hardware_concurrency() - 1;
    size_t chunckSize = ((modelsToLoadInfo.size() < maxThreadsCount) ?1 :modelsToLoadInfo.size() / maxThreadsCount);
    const size_t threadsCount = ((modelsToLoadInfo.size() < maxThreadsCount) ?modelsToLoadInfo.size() :maxThreadsCount);

    for (size_t i = 0; i < threadsCount; i++)
    {
        if (i == threadsCount - 1 && maxThreadsCount < modelsToLoadInfo.size())
        {
            chunckSize = (modelsToLoadInfo.size() - (threadsCount * chunckSize));
        }
        threads.push_back(std::thread(&Scene::loadModel,this,i,chunckSize,modelsToLoadInfo));
    }

    for (auto& thread : threads)
        thread.join();

    if (getRenderResource()->m_objectModelIndices.size() == 0)
        throw std::runtime_error("Add at least 1 model." );
    if (getRenderResource()->m_directionalLightIndex == -1)
        throw std::runtime_error("Add at least 1 directional light.");
    if (getRenderResource()->m_skyboxIndex == -1)
        throw std::runtime_error("Add at least 1 skybox.");
}

void Scene::loadModel(const size_t startI,const size_t chunckSize,const std::vector<ModelInfo>& modelsToLoadInfo) 
{
    const size_t endI = startI + chunckSize;

    for (size_t i = startI; i < endI; i++)
    {
        const ModelInfo& modelInfo = modelsToLoadInfo[i];

        switch (modelInfo.type)
        {
            case ModelType::SKYBOX:
            {
                getRenderResource()->m_modelResource.push_back(std::make_shared<Skybox>(modelInfo));
               
                if (getRenderResource()->m_skyboxIndex != -1)
                    throw std::runtime_error("You can't add more than 1 skybox per scene!");

                getRenderResource()->m_skyboxIndex = getRenderResource()->m_modelResource.size() - 1;
                m_skybox = std::dynamic_pointer_cast<Skybox>(getRenderResource()->m_modelResource[getRenderResource()->m_skyboxIndex]);

                break;

            }
            case ModelType::NORMAL_PBR:
            {
               
                getRenderResource()->m_modelResource.push_back(std::make_shared<NormalPBR>(modelInfo));
                getRenderResource()->m_objectModelIndices.push_back(getRenderResource()->m_modelResource.size() - 1);

                break;

            }
            case ModelType::LIGHT:
            {
                getRenderResource()->m_modelResource.push_back(std::make_shared<Light>(modelInfo));
                getRenderResource()->m_lightModelIndices.push_back(getRenderResource()->m_modelResource.size() - 1);

                if (modelInfo.lType == LightType::DIRECTIONAL_LIGHT)
                {
                    if (getRenderResource()->m_directionalLightIndex != -1)
                        throw std::runtime_error("You can't add more than 1 directional light per scene!");
                    getRenderResource()->m_directionalLightIndex = getRenderResource()->m_modelResource.size() - 1;
                }
                break;
            }
        }
    }
}


const Graphics& Scene::getPBRpipeline() const
{
    return m_graphicsPipelinePBR;
}

const Graphics& Scene::getSkyboxPipeline() const
{
    return m_graphicsPipelineSkybox;
}

const Graphics& Scene::getLightPipeline() const
{
    return m_graphicsPipelineLight;
}

void Scene::updateUBO(
    const std::shared_ptr<Camera>& camera,
    const glm::mat4& lightSpace,
    const VkExtent2D& extent,
    const uint32_t& currentFrame
) {
    UBOinfo uboInfo = {
        camera->getPos(),
        camera->getViewM(),
        camera->getProjectionM(),
        lightSpace,
         static_cast<uint32_t>(getRenderResource()->m_lightModelIndices.size()),
        extent
    };

    // Scene

    for (auto& model : getRenderResource()->m_modelResource)
    {
        model->updateUBO(currentFrame, uboInfo);

        if (auto pModel = std::dynamic_pointer_cast<NormalPBR>(model))
        {
            pModel->updateUBOlights(currentFrame);
        }
    }
}


const RenderPass& Scene::getRenderPass() const
{
    return m_renderPass;
}

void Scene::upload(
    const VkQueue& graphicsQueue,
    const VkCommandPool& commandPool,
    VkDescriptorPool& descriptorPool,
    // Features
    const std::shared_ptr<ShadowMap<Attributes::PBR::Vertex>> shadowMap
) {
    // First we upload the skybox because we need some dependencies from it for
    // the descriptor sets of the other models.
    m_skybox->upload(
        graphicsQueue,
        commandPool,
        Config::MAX_FRAMES_IN_FLIGHT
    );

    m_skybox->createDescriptorSets(
        m_graphicsPipelineSkybox.getDescriptorSetLayout(),
        {},
        descriptorPool
    );

    // IBL
    {
        loadBRDFlut(graphicsQueue, commandPool, m_vmaAllocator);


        m_prefilteredIrradiance = std::make_shared<PrefilteredIrradiance<Attributes::SKYBOX::Vertex>>(
            graphicsQueue,
            commandPool,
            Config::PREF_IRRADIANCE_DIM,
            m_skybox->getMeshes(),
            m_skybox->getEnvMap()
            );

        m_prefilteredEnvMap = std::make_shared<PrefilteredEnvMap<Attributes::SKYBOX::Vertex>>(
            graphicsQueue,
            commandPool,
            Config::PREF_ENV_MAP_DIM,
            m_skybox->getMeshes(),
            m_skybox->getEnvMap()
            );
    }

    //getRenderResource()->updateIBLResource(m_BRDFlut, m_prefilteredIrradiance->get(), m_prefilteredEnvMap->get());
    getRenderResource()->updateIBLResource(m_BRDFlut, m_prefilteredIrradiance->get(), m_prefilteredEnvMap->get());


    std::vector<VkDescriptorImageInfo*>  additionalImage = {
        &(getRenderResource()->m_IBLResource.irradiance->getDescriptorImageInfo()) ,
        &(getRenderResource()->m_IBLResource.brdfLUT->getDescriptorImageInfo()),
        &(getRenderResource()->m_IBLResource.prefiltered_Env->getDescriptorImageInfo()),
        &(shadowMap->get()->getDescriptorImageInfo())
    };


    VkDescriptorSetLayout descriptorSetLayout;
    for (auto& model : getRenderResource()->m_modelResource)
    {
        auto type = model->getType();
        if (type == ModelType::SKYBOX) continue;

        model->upload(graphicsQueue, commandPool, Config::MAX_FRAMES_IN_FLIGHT);
        if (type == ModelType::NORMAL_PBR)
            descriptorSetLayout = (m_graphicsPipelinePBR.getDescriptorSetLayout());
        else
            if (type == ModelType::LIGHT)
                descriptorSetLayout = (m_graphicsPipelineLight.getDescriptorSetLayout());

        model->createDescriptorSets(descriptorSetLayout, additionalImage, descriptorPool);
    }
}


void Scene::destroy()
{
    for (auto& model : getRenderResource()->m_modelResource)
        model->destroy();

    m_graphicsPipelinePBR.destroy();
    m_graphicsPipelineSkybox.destroy();
    m_graphicsPipelineLight.destroy();

    m_renderPass.destroy();

    // IBL
    m_BRDFcomp.destroy();
    m_BRDFlut->destroy();
    m_prefilteredIrradiance->destroy();
    m_prefilteredEnvMap->destroy();
}

void Scene::loadBRDFlut(
    const VkQueue& graphicsQueue,
    const VkCommandPool& commandPool,
    const VmaAllocator& vmaAllocator
) {
   
    std::string TextureName = "BRDF_LUT.tga";
    TextureToLoadInfo info = { TextureName,"/defaultTextures",VK_FORMAT_R8G8B8A8_SRGB,4};

    m_BRDFlut = std::make_shared<NormalTexture>(
            TextureName,
            std::string(MODEL_DIR) + info.folderName ,
            info.format
        );

}

// In the future, it'll return a vector of computations.
const Computation& Scene::getComputation() const
{
    return m_BRDFcomp;
}


