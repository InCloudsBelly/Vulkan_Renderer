#include "VulkanRenderer/Scene/Scene.h"

#include <thread>
#include <iostream>

#include "VulkanRenderer/Renderer.h"
#include "VulkanRenderer/Buffer/BufferManager.h"
#include "VulkanRenderer/Math/MathUtils.h"

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

void Scene::createUniformBuffer(const std::shared_ptr<Model> modelPtr, std::vector<size_t>& uboSizeInfos)
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

void Scene::createDescriptorSet(const std::shared_ptr<Model> modelPtr, const VkDescriptorPool& descriptorPool, const VkDescriptorSetLayout& layout, std::vector<VkDescriptorImageInfo*> additionalImages, const std::vector<DescriptorInfo>& descriptorInfos)
{
    for (uint32_t meshIndex : modelPtr->getMeshIndices())
    {
        //create DescriptorSet PerMesh
        DescriptorManager::allocDescriptorSet(descriptorPool, layout, &m_descriptorSetsMap[meshIndex]);

        RenderMeshInfo& renderMeshInfo = getRenderResource()->m_meshInfoMap[meshIndex];
        std::vector<std::shared_ptr<TextureBase>> materialTextures;
        if (renderMeshInfo.ref_material != nullptr)
        {
            materialTextures = {
                renderMeshInfo.ref_material->colorTexture,
                renderMeshInfo.ref_material->metallic_RoughnessTexture,
                renderMeshInfo.ref_material->emissiveTexture,
                renderMeshInfo.ref_material->AOTexture,
                renderMeshInfo.ref_material->normalTexture
            };
        }
        else
            materialTextures = {};

        DescriptorManager::createDescriptorSet(
            descriptorInfos,
            materialTextures,
            additionalImages,
            m_ubosMap[meshIndex],
            &m_descriptorSetsMap[meshIndex]
        );
    }
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
        GRAPHICS_PIPELINE::LIGHT::DESCRIPTORS_INFO,
        {}
    );
}


void Scene::loadModels(const std::vector<ModelInfo>& modelsToLoadInfo)
{
    std::vector<std::thread> threads;

    const uint32_t maxThreadsCount = std::thread::hardware_concurrency() - 1;
    uint32_t chunckSize = ((modelsToLoadInfo.size() < maxThreadsCount) ?1 :modelsToLoadInfo.size() / maxThreadsCount);
    const uint32_t threadsCount = ((modelsToLoadInfo.size() < maxThreadsCount) ?modelsToLoadInfo.size() :maxThreadsCount);

    for (uint32_t i = 0; i < threadsCount; i++)
    {
        if (i == threadsCount - 1 && maxThreadsCount < modelsToLoadInfo.size())
        {
            chunckSize = (modelsToLoadInfo.size() - (threadsCount * chunckSize));
        }
        threads.push_back(std::thread(&Scene::loadModel,this,i,chunckSize,modelsToLoadInfo));
    }

    for (auto& thread : threads)
        thread.join();

  /*  if (getRenderResource()->m_objectModelIndices.size() == 0)
        throw std::runtime_error("Add at least 1 model." );
    if (getRenderResource()->m_directionalLightIndex == -1)
        throw std::runtime_error("Add at least 1 directional light.");
    if (getRenderResource()->m_skyboxIndex == -1)
        throw std::runtime_error("Add at least 1 skybox.");*/
}

void Scene::loadModel(const uint32_t startI,const uint32_t chunckSize,const std::vector<ModelInfo>& modelsToLoadInfo) 
{
    const uint32_t endI = startI + chunckSize;

    for (uint32_t i = startI; i < endI; i++)
    {
        const ModelInfo& modelInfo = modelsToLoadInfo[i];

        std::shared_ptr<Model> model = std::make_shared<Model>(modelInfo.name, modelInfo.fileName, modelInfo.folderName, modelInfo.type, glm::fvec4(modelInfo.pos, 1.0f), modelInfo.rot, modelInfo.size);
       
        if (modelInfo.type == ModelType::NORMAL_PBR)
            getRenderResource()->m_normalModels.push_back(model);
        else if (modelInfo.type == ModelType::SKYBOX)
            getRenderResource()->m_skybox = model;
        else if (modelInfo.type == ModelType::LIGHT)
        {
            getRenderResource()->m_lightModels.push_back(model);

            LightInfo lightInfo{ modelInfo.name,glm::fvec4(modelInfo.pos, 1.0f), modelInfo.rot, modelInfo.size, glm::fvec4(modelInfo.endPos, 1.0f), glm::fvec4(modelInfo.color,1.0f), 1.0f, modelInfo.lType, model };
            lightInfo.m_intensity = lightInfo.m_lightType == LightType::DIRECTIONAL_LIGHT ? 15.0f : 70.0f;
            getRenderResource()->m_lightsInfo.push_back(lightInfo);
            if (lightInfo.m_lightType == LightType::DIRECTIONAL_LIGHT)
                getRenderResource()->m_directionalLightIndex = getRenderResource()->m_lightsInfo.size() - 1;
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
        getRenderResource()->m_lightsInfo.size(),
        extent
    };

    // Scene

    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            // update normal UBO 
            DescriptorTypes::UniformBufferObject::NormalPBR  uboData1;
            uboData1.model = ptr->getModelMatrix();
            uboData1.view = uboInfo.view;
            uboData1.proj = uboInfo.proj;
            uboData1.lightSpace = uboInfo.lightSpace;

            uboData1.cameraPos = uboInfo.cameraPos;
            uboData1.lightsCount = uboInfo.lightsCount;

            void* data1;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0], &data1);
            memcpy(data1, &uboData1, sizeof(uboData1));
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0]);


            // update lights UBO
            DescriptorTypes::UniformBufferObject::LightInfo uboData2[Config::LIGHTS_COUNT];

            for (uint32_t i = 0; i < getRenderResource()->m_lightsInfo.size(); i++)
            {
                LightInfo info = getRenderResource()->m_lightsInfo[i];
                uboData2[i].pos = info.pos;
                uboData2[i].color = info.m_color;
                uboData2[i].dir = info.m_targetPos - info.pos;
                uboData2[i].intensity = info.m_intensity;
                uboData2[i].type = (int)info.m_lightType;
            }

            void* data2;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][1], &data2);
            memcpy(data2, &uboData2, sizeof(uboData2[0]) * 10);
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][1]);
        }
    }

    for (auto ptr : getRenderResource()->m_lightModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            DescriptorTypes::UniformBufferObject::Light newUBO;
            newUBO.model = ptr->getModelMatrix();

            newUBO.view = uboInfo.view;
            newUBO.proj = uboInfo.proj;
            newUBO.lightColor = glm::fvec4(1.0f);

            void* data;
            vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0], &data);
            memcpy(data, &newUBO, sizeof(newUBO));
            vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0]);
        }
    }
 
    auto skybox = getRenderResource()->m_skybox;
    for (uint32_t meshIndex : skybox->getMeshIndices())
    {
        DescriptorTypes::UniformBufferObject::Skybox newUBO;

        newUBO.model = glm::translate(glm::mat4(1.0f), glm::vec3(uboInfo.cameraPos));
        newUBO.view = uboInfo.view;
        newUBO.proj = MathUtils::getUpdatedProjMatrix(glm::radians(75.0f), uboInfo.extent.width / (float)uboInfo.extent.height, 0.01f, 40.0f);

        void* data;
        vmaMapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0], &data);
        memcpy(data, &newUBO, sizeof(newUBO));
        vmaUnmapMemory(getRendererPointer()->getVmaAllocator(), m_uboAllocationsMap[meshIndex][0]);
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
    const std::shared_ptr<ShadowMap<MeshVertex>> shadowMap
) {

    for (auto ptr : getRenderResource()->m_normalModels)
    {
        ptr->upload(commandPool, graphicsQueue);
    }

    for (auto ptr : getRenderResource()->m_lightModels)
    {
        ptr->upload(commandPool, graphicsQueue);
    }

    auto skybox = getRenderResource()->m_skybox;
    skybox->upload(commandPool, graphicsQueue);


    // IBL
    {
        loadBRDFlut(graphicsQueue, commandPool, m_vmaAllocator);

        m_prefilteredIrradiance = std::make_shared<PrefilteredIrradiance>(
            graphicsQueue,
            commandPool,
            Config::PREF_IRRADIANCE_DIM,
            getRenderResource()->m_skyboxCubeMap
            );

        m_prefilteredEnvMap = std::make_shared<PrefilteredEnvMap>(
            graphicsQueue,
            commandPool,
            Config::PREF_ENV_MAP_DIM,
            getRenderResource()->m_skyboxCubeMap
            );
    }

    getRenderResource()->updateIBLResource(m_BRDFlut, m_prefilteredIrradiance->get(), m_prefilteredEnvMap->get());


    std::vector<VkDescriptorImageInfo*>  additionalImage = {
        &(getRenderResource()->m_IBLResource.irradiance->getDescriptorImageInfo()) ,
        &(getRenderResource()->m_IBLResource.brdfLUT->getDescriptorImageInfo()),
        &(getRenderResource()->m_IBLResource.prefiltered_Env->getDescriptorImageInfo()),
        &(shadowMap->get()->getDescriptorImageInfo())
    };


    for (auto ptr : getRenderResource()->m_normalModels)
    {
        std::vector<size_t> uboSizeInfos = {
               sizeof(DescriptorTypes::UniformBufferObject::NormalPBR),
               sizeof(DescriptorTypes::UniformBufferObject::LightInfo) * 10
        };
        createUniformBuffer(ptr, uboSizeInfos);

        createDescriptorSet(ptr, descriptorPool, m_graphicsPipelinePBR.getDescriptorSetLayout(), additionalImage, GRAPHICS_PIPELINE::PBR::DESCRIPTORS_INFO);
    }

    for (auto ptr : getRenderResource()->m_lightModels)
    {
        std::vector<size_t> uboSizeInfos = {sizeof(DescriptorTypes::UniformBufferObject::Light)};
        createUniformBuffer(ptr, uboSizeInfos);
        createDescriptorSet(ptr, descriptorPool, m_graphicsPipelineLight.getDescriptorSetLayout(), { &getRenderResource()->m_defaultTexture->getDescriptorImageInfo() }, GRAPHICS_PIPELINE::LIGHT::DESCRIPTORS_INFO);
    }

    {
        auto skybox = getRenderResource()->m_skybox;
        std::vector<size_t> uboSizeInfos = { sizeof(DescriptorTypes::UniformBufferObject::Skybox) };
        createUniformBuffer(skybox, uboSizeInfos);
        createDescriptorSet(skybox, descriptorPool, m_graphicsPipelineSkybox.getDescriptorSetLayout(), { &getRenderResource()->m_skyboxCubeMap->getDescriptorImageInfo() }, GRAPHICS_PIPELINE::SKYBOX::DESCRIPTORS_INFO);
    }
}


void Scene::destroy()
{
    for (auto ptr : getRenderResource()->m_normalModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            for (uint32_t i = 0; i < m_ubosMap[meshIndex].size(); ++i)
            {
                vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_ubosMap[meshIndex][i], m_uboAllocationsMap[meshIndex][i]);
            }
        }
    }

    for (auto ptr : getRenderResource()->m_lightModels)
    {
        for (uint32_t meshIndex : ptr->getMeshIndices())
        {
            for (uint32_t i = 0; i < m_ubosMap[meshIndex].size(); ++i)
            {
                vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_ubosMap[meshIndex][i], m_uboAllocationsMap[meshIndex][i]);
            }
        }
    }

    auto skybox = getRenderResource()->m_skybox;
    for (uint32_t meshIndex : skybox->getMeshIndices())
    {
        for (uint32_t i = 0; i < m_ubosMap[meshIndex].size(); ++i)
        {
            vmaDestroyBuffer(getRendererPointer()->getVmaAllocator(), m_ubosMap[meshIndex][i], m_uboAllocationsMap[meshIndex][i]);
        }
    }


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
   
    std::string TextureName ="BRDF_LUT.png";
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


