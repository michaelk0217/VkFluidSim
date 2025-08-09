#include "FluidSimulator.h"

#include "VulkanBuffer.h"
#include "VulkanCommandBuffers.h"
#include "utilities.hpp"

//#include "VulkanInitializers.hpp"

FluidSimulator::FluidSimulator(
    VulkanDevices& devices,
    //VulkanPipelineLayout& pipelineLayout,
    VkCommandPool commandPool,
    VkFormat colorFormat,
    VkDescriptorPool descriptorPool,
    std::vector<VkBuffer> uboBuffers
    //VulkanDescriptorSets& descriptorSets
) :
    m_devices(devices),
    m_commandPool(commandPool),
    m_descriptorPool(descriptorPool)
    //m_graphicsDescriptorSets(descriptorSets)
    //m_pipelineLayout(pipelineLayout)
{

    m_boxVertices = {
        {{-m_params.boxHalfWidth, -m_params.boxHalfHeight, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{m_params.boxHalfWidth, -m_params.boxHalfHeight, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{m_params.boxHalfWidth, m_params.boxHalfHeight, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-m_params.boxHalfWidth, m_params.boxHalfHeight, 0.0f}, {1.0f, 1.0f, 1.0f}}
    };

    createGraphicsPipelineLayout();

    graphicsDescriptorSets = std::make_unique<VulkanDescriptorSets>();
    graphicsDescriptorSets->create(devices.getLogicalDevice(), descriptorPool, graphicsDescriptorSetLayout, 2);
    
    m_particlePipeline = std::make_unique<VulkanGraphicsPipeline>();
    m_particlePipeline->createDynamic(
        devices.getLogicalDevice(),
        devices.getPhysicalDevice(),
        graphicsPipelineLayout,
        colorFormat,
        "shaders/particle.vert.spv",
        "shaders/particle.frag.spv"
    );

    m_boxDescriptorSetLayout = std::make_unique<VulkanDescriptorSetLayout>(devices.getLogicalDevice());
    m_boxPipelineLayout = std::make_unique<VulkanPipelineLayout>(devices.getLogicalDevice(), m_boxDescriptorSetLayout->getDescriptorSetLayout());
    m_boxDescriptorSets = std::make_unique<VulkanDescriptorSets>();
    m_boxDescriptorSets->create(devices.getLogicalDevice(), descriptorPool, m_boxDescriptorSetLayout->getDescriptorSetLayout(), 2);
    m_boxDescriptorSets->updateGraphicsBoxDescriptorSet(uboBuffers);

    m_boxPipeline = std::make_unique <VulkanGraphicsPipeline>();
    m_boxPipeline->createLiquidBoxPipeline(
        devices.getLogicalDevice(),
        devices.getPhysicalDevice(),
        m_boxPipelineLayout->getPipelineLayout(),
        colorFormat,
        "shaders/box.vert.spv",
        "shaders/box.frag.spv"
    );
    m_boxVertexBuffer = std::make_unique<VulkanVertexBuffer>();
    m_boxVertexBuffer->createCoherent(devices.getLogicalDevice(), devices.getPhysicalDevice(), sizeof(Vertex) * 4);
    void* boxData = m_boxVertexBuffer->map();
    memcpy(boxData, m_boxVertices.data(), m_boxVertices.size() * sizeof(Vertex));
    m_boxVertexBuffer->unmap();
    m_boxIndexBuffer = std::make_unique<VulkanIndexBuffer>();
    m_boxIndexBuffer->create(devices.getLogicalDevice(), devices.getPhysicalDevice(), devices.getGraphicsQueue(), commandPool, m_boxIndices);
    
    initializeParticles();

    createParticleBuffers();

    createHashPassResources();
    createSortPassResources();
    createIndexingPassResources();
    createComputePipeline();
    createComputeDescriptorSets();


}

FluidSimulator::~FluidSimulator()
{
    if (graphicsDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_devices.getLogicalDevice(), graphicsDescriptorSetLayout, nullptr);
    }
    if (graphicsPipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_devices.getLogicalDevice(), graphicsPipelineLayout, nullptr);
    }
    if (m_fluidComputePipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_devices.getLogicalDevice(), m_fluidComputePipeline, nullptr);
    }
    if (m_fluidComputePipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_devices.getLogicalDevice(), m_fluidComputePipelineLayout, nullptr);
    }
    if (m_fluidComputeDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_devices.getLogicalDevice(), m_fluidComputeDescriptorSetLayout, nullptr);
    }
    if (!particleBuffers.empty()) {
        for (auto& pBuff : particleBuffers)
        {
            vkDestroyBuffer(m_devices.getLogicalDevice(), pBuff, nullptr);
        }
        particleBuffers.clear();
    }
    if (!particleBufferMemories.empty())
    {
        for (auto& pBuffMem : particleBufferMemories)
        {
            vkFreeMemory(m_devices.getLogicalDevice(), pBuffMem, nullptr);
        }
        particleBufferMemories.clear();
    }

    // hash resources
    if (m_hashPipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_devices.getLogicalDevice(), m_hashPipeline, nullptr);
    if (m_hashPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_devices.getLogicalDevice(), m_hashPipelineLayout, nullptr);
    if (m_hashDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(m_devices.getLogicalDevice(), m_hashDescriptorSetLayout, nullptr);
    if (m_particleHashesBuffer != VK_NULL_HANDLE) vkDestroyBuffer(m_devices.getLogicalDevice(), m_particleHashesBuffer, nullptr);
    if (m_particleIndicesBuffer != VK_NULL_HANDLE) vkDestroyBuffer(m_devices.getLogicalDevice(), m_particleIndicesBuffer, nullptr);
    if (m_particleHashesMemory != VK_NULL_HANDLE) vkFreeMemory(m_devices.getLogicalDevice(), m_particleHashesMemory, nullptr);
    if (m_particleIndicesMemory != VK_NULL_HANDLE) vkFreeMemory(m_devices.getLogicalDevice(), m_particleIndicesMemory, nullptr);

    // sort resources
    if (m_sortPipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_devices.getLogicalDevice(), m_sortPipeline, nullptr);
    if (m_sortPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_devices.getLogicalDevice(), m_sortPipelineLayout, nullptr);
    if (m_sortDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(m_devices.getLogicalDevice(), m_sortDescriptorSetLayout, nullptr);
    
    // calc indices resources
    if (m_cellStartIndicesBuffer != VK_NULL_HANDLE) vkDestroyBuffer(m_devices.getLogicalDevice(), m_cellStartIndicesBuffer, nullptr);
    if (m_cellStartIndicesMemory != VK_NULL_HANDLE) vkFreeMemory(m_devices.getLogicalDevice(), m_cellStartIndicesMemory, nullptr);
    if (m_indexingDescriptorSetLayout != VK_NULL_HANDLE) vkDestroyDescriptorSetLayout(m_devices.getLogicalDevice(), m_indexingDescriptorSetLayout, nullptr);
    if (m_indexingPipelineLayout != VK_NULL_HANDLE) vkDestroyPipelineLayout(m_devices.getLogicalDevice(), m_indexingPipelineLayout, nullptr);
    if (m_indexingPipeline != VK_NULL_HANDLE) vkDestroyPipeline(m_devices.getLogicalDevice(), m_indexingPipeline, nullptr);
}

void FluidSimulator::update(float deltaTime, VkCommandBuffer commandBuffer, glm::vec2 mousePos)
{
    bool did_reset = m_params.resetSimulation;

    if (did_reset)
    {
        //std::cout << "reseting sim" << std::endl;
        vkDeviceWaitIdle(m_devices.getLogicalDevice());

        for (auto& buffer : particleBuffers)
        {
            vkDestroyBuffer(m_devices.getLogicalDevice(), buffer, nullptr);
        }
        for (auto& memory : particleBufferMemories)
        {
            vkFreeMemory(m_devices.getLogicalDevice(), memory, nullptr);
        }

        initializeParticles();

        createParticleBuffers();
        updateHashDescriptorSets();
        updateComputeDescriptorSets();
        currentBuffer = 0;

        m_params.resetSimulation = false;
    }

    updateContainerBuffer();



    if (m_params.runSimulation || did_reset)
    {
        ComputeParameters push{};
        push.particleCount = m_params.particleCount;
        push.deltaTime = did_reset ? 0.0f : deltaTime;
        push.gravity = m_params.gravity;
        push.smoothingRadius = m_params.smoothingRadius;
        push.targetDensity = m_params.targetDensity;
        push.pressureMultiplier = m_params.pressureMultiplier;
        push.collisionDamping = m_params.collisionDamping;
        push.mass = m_params.mass;
        push.boxHalfSize = glm::vec2(m_params.boxHalfWidth, m_params.boxHalfHeight);
        push.mousePos = mousePos;
        push.color1 = glm::vec4(m_params.colorPoints.color1, 1.0f);
        push.color2 = glm::vec4(m_params.colorPoints.color2, 1.0f);
        push.color3 = glm::vec4(m_params.colorPoints.color3, 1.0f);
        push.maxSpeedForColor = m_params.maxSpeedForColor;

        uint32_t groupCountX = (m_params.particleCount + 255) / 256;

        // --- PRE-COMPUTE ---
        // clear cell start indices to sential value (0xFFFFFFFF = -1), marking all values empty
        const uint32_t SENTINEL = 0xFFFFFFFF;
        vkCmdFillBuffer(commandBuffer, m_particleHashesBuffer, 0, VK_WHOLE_SIZE, SENTINEL);
        vkCmdFillBuffer(commandBuffer, m_particleIndicesBuffer, 0, VK_WHOLE_SIZE, SENTINEL);
        vkCmdFillBuffer(commandBuffer, m_cellStartIndicesBuffer, 0, VK_WHOLE_SIZE, SENTINEL);



        // --- Dispatch Hash Pass ---
        vks::tools::insertMemoryBarrier2(
            commandBuffer,
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_TRANSFER_WRITE_BIT,
            VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_TRANSFER_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
        );

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_hashPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_hashPipelineLayout, 0, 1, &m_hashDescriptorSets[currentBuffer], 0, nullptr);
        vkCmdPushConstants(commandBuffer, m_hashPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeParameters), &push);
        vkCmdDispatch(commandBuffer, groupCountX, 1, 1);

        // --- Dispatch Sort Pass ---
        vks::tools::insertMemoryBarrier2(
            commandBuffer,
            VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
        );

        dispatchSort(commandBuffer);

        // --- Dispatch Indexing Pass ---
        vks::tools::insertMemoryBarrier2(
            commandBuffer,
            VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
        );

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_indexingPipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_indexingPipelineLayout, 0, 1, &m_indexingDescriptorSet, 0, nullptr);
        vkCmdPushConstants(commandBuffer, m_indexingPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeParameters), &push);
        vkCmdDispatch(commandBuffer, groupCountX, 1, 1);

        // --- Dispatch Simulation Pass ---
        vks::tools::insertMemoryBarrier2(
            commandBuffer,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_ACCESS_2_SHADER_READ_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT
        );

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fluidComputePipeline);
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_fluidComputePipelineLayout, 0, 1, &m_fluidComputeDescriptorSets[currentBuffer], 0, nullptr);
        vkCmdPushConstants(commandBuffer, m_fluidComputePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeParameters), &push);
        vkCmdDispatch(commandBuffer, groupCountX, 1, 1);

        vks::tools::insertMemoryBarrier2(
            commandBuffer,
            VK_ACCESS_2_SHADER_WRITE_BIT | VK_ACCESS_2_SHADER_WRITE_BIT,
            VK_ACCESS_2_SHADER_READ_BIT,
            VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT,
            VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT
        );

        currentBuffer = 1 - currentBuffer;
    }
}

void FluidSimulator::draw(VkCommandBuffer commandBuffer, uint32_t currentFrameIndex, VkBuffer uboBuffer)
{
    graphicsDescriptorSets->updateGraphicsDescriptorSet(currentFrameIndex, particleBuffers[currentBuffer], uboBuffer);
    // bind graphics pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_particlePipeline->getGraphicsPipeline());
    // bind trainge vertex buffer (contains position and colors)
    
    //VkBuffer vBuff = m_particleVertexBuffer->getVkBuffer();
    //vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vBuff, offsets);

    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        graphicsPipelineLayout,
        0, 1,
        &graphicsDescriptorSets->getDescriptorSets()[currentFrameIndex],
        0, nullptr
    );


    vkCmdDraw(commandBuffer, m_params.particleCount, 1, 0, 0);

    // ----- CONTAINER ------
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_boxPipeline->getGraphicsPipeline());
    vkCmdBindDescriptorSets(
        commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        m_boxPipelineLayout->getPipelineLayout(),
        0, 1,
        &m_boxDescriptorSets->getDescriptorSets()[currentFrameIndex],
        0, nullptr
    );

    vkCmdSetLineWidth(commandBuffer, 1.0f);
    VkBuffer vBuff = m_boxVertexBuffer->getVkBuffer();
    VkBuffer iBuff = m_boxIndexBuffer->getVkBuffer();
    VkDeviceSize offsets[1]{ 0 };
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vBuff, offsets);
    vkCmdBindIndexBuffer(commandBuffer, iBuff, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_boxIndices.size()), 1, 0, 0, 0);
}

void FluidSimulator::createHashPassResources()
{
    VkDeviceSize bufferSize = sizeof(uint32_t) * MAX_PARTICLES;

    // holds calculated hash on gpu
    VulkanBuffer::createBuffer(
        m_devices.getLogicalDevice(),
        m_devices.getPhysicalDevice(),
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_particleHashesBuffer,
        m_particleHashesMemory
    );

    // holds original index of each particle
    VulkanBuffer::createBuffer(
        m_devices.getLogicalDevice(),
        m_devices.getPhysicalDevice(),
        bufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_particleIndicesBuffer,
        m_particleIndicesMemory
    );

    // hash descriptor set layout (input: particles, output: hashes, indices)
    std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo hashDescriptorSetLayoutCI{};
    hashDescriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    hashDescriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    hashDescriptorSetLayoutCI.pBindings = layoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_devices.getLogicalDevice(), &hashDescriptorSetLayoutCI, nullptr, &m_hashDescriptorSetLayout));

    // hash pipeline layout
    VkPipelineLayoutCreateInfo hashPipelineLayoutCI{};
    hashPipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    hashPipelineLayoutCI.setLayoutCount = 1;
    hashPipelineLayoutCI.pSetLayouts = &m_hashDescriptorSetLayout;
    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(ComputeParameters);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    hashPipelineLayoutCI.pushConstantRangeCount = 1;
    hashPipelineLayoutCI.pPushConstantRanges = &pushConstant;

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_devices.getLogicalDevice(), &hashPipelineLayoutCI, nullptr, &m_hashPipelineLayout));

    // hash compute pipeline
    VkShaderModule computeShaderModule = vks::tools::loadShader("shaders/hash.comp.spv", m_devices.getLogicalDevice());
    
    VkPipelineShaderStageCreateInfo shaderStageCI{};
    shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCI.module = computeShaderModule;
    shaderStageCI.pName = "main";

    VkComputePipelineCreateInfo computePipelineCI{};
    computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCI.layout = m_hashPipelineLayout;
    computePipelineCI.stage = shaderStageCI;
    VK_CHECK_RESULT(vkCreateComputePipelines(m_devices.getLogicalDevice(), VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &m_hashPipeline));

    vkDestroyShaderModule(m_devices.getLogicalDevice(), computeShaderModule, nullptr);

    // hash descriptor sets
    std::vector<VkDescriptorSetLayout> layouts(2, m_hashDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 2;
    allocInfo.pSetLayouts = layouts.data();

    m_hashDescriptorSets.resize(2);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(m_devices.getLogicalDevice(), &allocInfo, m_hashDescriptorSets.data()));
    
    updateHashDescriptorSets();
}

void FluidSimulator::updateHashDescriptorSets()
{
    for (size_t i = 0; i < 2; i++)
    {
        VkDescriptorBufferInfo particleBufferInfo{};
        particleBufferInfo.buffer = particleBuffers[i];
        particleBufferInfo.offset = 0;
        particleBufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo hashesBufferInfo{};
        hashesBufferInfo.buffer = m_particleHashesBuffer;
        hashesBufferInfo.offset = 0;
        hashesBufferInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo indicesBufferInfo{};
        indicesBufferInfo.buffer = m_particleIndicesBuffer;
        indicesBufferInfo.offset = 0;
        indicesBufferInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_hashDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &particleBufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_hashDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &hashesBufferInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = m_hashDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &indicesBufferInfo;

        vkUpdateDescriptorSets(m_devices.getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    
}

void FluidSimulator::createSortPassResources()
{
    // --- sort descirptor set layout ---
    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings{};
    // keys to sort (particle hashes)
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // values to sort alongside the keys (particle indices)
    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
    descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    descriptorSetLayoutCI.pBindings = layoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_devices.getLogicalDevice(), &descriptorSetLayoutCI, nullptr, &m_sortDescriptorSetLayout));

    // --- sort pipeline layout ---
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = 1;
    pipelineLayoutCI.pSetLayouts = &m_sortDescriptorSetLayout;
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(SortPushConstants);
    pipelineLayoutCI.pushConstantRangeCount = 1;
    pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(m_devices.getLogicalDevice(), &pipelineLayoutCI, nullptr, &m_sortPipelineLayout));

    // --- sort compute pipeline ---
    VkShaderModule computeShaderModule = vks::tools::loadShader("shaders/sort.comp.spv", m_devices.getLogicalDevice());
    VkPipelineShaderStageCreateInfo shaderStageCI{};
    shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCI.module = computeShaderModule;
    shaderStageCI.pName = "main";

    VkComputePipelineCreateInfo computePipelineCI{};
    computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCI.layout = m_sortPipelineLayout;
    computePipelineCI.stage = shaderStageCI;
    VK_CHECK_RESULT(vkCreateComputePipelines(m_devices.getLogicalDevice(), VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &m_sortPipeline));
    vkDestroyShaderModule(m_devices.getLogicalDevice(), computeShaderModule, nullptr);

    // --- descriptor set ---
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_sortDescriptorSetLayout;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(m_devices.getLogicalDevice(), &allocInfo, &m_sortDescriptorSet));
    
    VkDescriptorBufferInfo hashesBufferInfo{};
    hashesBufferInfo.buffer = m_particleHashesBuffer;
    hashesBufferInfo.offset = 0;
    hashesBufferInfo.range = VK_WHOLE_SIZE;
    VkDescriptorBufferInfo indicesBufferInfo{};
    indicesBufferInfo.buffer = m_particleIndicesBuffer;
    indicesBufferInfo.offset = 0;
    indicesBufferInfo.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_sortDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &hashesBufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_sortDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &indicesBufferInfo;
    
    vkUpdateDescriptorSets(m_devices.getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void FluidSimulator::dispatchSort(VkCommandBuffer commandBuffer)
{
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_sortPipeline);
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, m_sortPipelineLayout, 0, 1, &m_sortDescriptorSet, 0, nullptr);

    VkMemoryBarrier2 computeToComputeBarrier{};
    computeToComputeBarrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER_2;
    computeToComputeBarrier.srcStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    computeToComputeBarrier.srcAccessMask = VK_ACCESS_2_SHADER_WRITE_BIT;
    computeToComputeBarrier.dstStageMask = VK_PIPELINE_STAGE_2_COMPUTE_SHADER_BIT;
    computeToComputeBarrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;

    VkDependencyInfo dependencyInfo{};
    dependencyInfo.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
    dependencyInfo.memoryBarrierCount = 1;
    dependencyInfo.pMemoryBarriers = &computeToComputeBarrier;

    uint32_t numParticles = m_params.particleCount;

    uint32_t paddedNumParticles = utils::nextPowerOf2(numParticles);
    const uint32_t workGroupSize = 256;
    uint32_t numWorkGroups = (paddedNumParticles + workGroupSize - 1) / workGroupSize;

    for (uint32_t mergeSize = 2; mergeSize <= paddedNumParticles; mergeSize *= 2) 
    {
        for (uint32_t compareStride = mergeSize / 2; compareStride > 0; compareStride /= 2) 
        {
            SortPushConstants push{};
            push.elementCount = paddedNumParticles;
            push.mergeSize = mergeSize;
            push.compareStride = compareStride;
            vkCmdPushConstants(commandBuffer, m_sortPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SortPushConstants), &push);

            vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);
            vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
        }
    }


 /*   for (uint32_t stage = 0; stage < numStages; ++stage) 
    {
        for (uint32_t pass = 0; pass < stage + 1; ++pass) 
        {
            SortPushConstants push{};
            push.elementCount = paddedNumParticles;
            push.stage = stage;
            push.pass = pass;

            vkCmdPushConstants(commandBuffer, m_sortPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SortPushConstants), &push);
            
            
            vkCmdDispatch(commandBuffer, numWorkGroups, 1, 1);
            vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
        }
    }*/


    //// Keep your existing push constants structure
    //SortPushConstants push{};
    //push.count = numParticles;

    //uint32_t groupCountX = (numParticles + 255) / 256;

    //// Simple bitonic sort - just fix the loop logic
    //for (uint32_t length = 2; length <= utils::nextPowerOf2(numParticles); length *= 2) {
    //    for (uint32_t inc = length / 2; inc > 0; inc /= 2) {
    //        push.groupWidth = inc;
    //        push.groupHeight = length;

    //        vkCmdPushConstants(commandBuffer, m_sortPipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(SortPushConstants), &push);
    //        vkCmdDispatch(commandBuffer, groupCountX, 1, 1);
    //        vkCmdPipelineBarrier2(commandBuffer, &dependencyInfo);
    //    }
    //}
}

void FluidSimulator::createIndexingPassResources()
{
    // --- buffer to store index list ---
    VulkanBuffer::createBuffer(
        m_devices.getLogicalDevice(),
        m_devices.getPhysicalDevice(),
        sizeof(uint32_t) * HASH_TABLE_SIZE,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        m_cellStartIndicesBuffer,
        m_cellStartIndicesMemory
    );

    // --- descriptor set layout ---
    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings{};
    // input sorted particle hashes
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // output cell start indices
    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCI{};
    descriptorSetLayoutCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    descriptorSetLayoutCI.bindingCount = static_cast<uint32_t>(layoutBindings.size());
    descriptorSetLayoutCI.pBindings = layoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_devices.getLogicalDevice(), &descriptorSetLayoutCI, nullptr, &m_indexingDescriptorSetLayout));

    // --- pipeline layout ---
    VkPipelineLayoutCreateInfo pipelineLayoutCI{};
    pipelineLayoutCI.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutCI.setLayoutCount = 1;
    pipelineLayoutCI.pSetLayouts = &m_indexingDescriptorSetLayout;
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(ComputeParameters);
    pipelineLayoutCI.pushConstantRangeCount = 1;
    pipelineLayoutCI.pPushConstantRanges = &pushConstantRange;
    VK_CHECK_RESULT(vkCreatePipelineLayout(m_devices.getLogicalDevice(), &pipelineLayoutCI, nullptr, &m_indexingPipelineLayout));

    // --- compute pipeline ---
    VkShaderModule computeShaderModule = vks::tools::loadShader("shaders/calc_indices.comp.spv", m_devices.getLogicalDevice());
    VkPipelineShaderStageCreateInfo shaderStageCI{};
    shaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageCI.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageCI.module = computeShaderModule;
    shaderStageCI.pName = "main";

    VkComputePipelineCreateInfo computePipelineCI{};
    computePipelineCI.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineCI.layout = m_indexingPipelineLayout;
    computePipelineCI.stage = shaderStageCI;
    VK_CHECK_RESULT(vkCreateComputePipelines(m_devices.getLogicalDevice(), VK_NULL_HANDLE, 1, &computePipelineCI, nullptr, &m_indexingPipeline));
    vkDestroyShaderModule(m_devices.getLogicalDevice(), computeShaderModule, nullptr);

    // --- descriptor set ---
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_indexingDescriptorSetLayout;
    VK_CHECK_RESULT(vkAllocateDescriptorSets(m_devices.getLogicalDevice(), &allocInfo, &m_indexingDescriptorSet));

    VkDescriptorBufferInfo hashesBufferInfo{};
    hashesBufferInfo.buffer = m_particleHashesBuffer;
    hashesBufferInfo.offset = 0;
    hashesBufferInfo.range = VK_WHOLE_SIZE;
    VkDescriptorBufferInfo cellIndicesBufferInfo{};
    cellIndicesBufferInfo.buffer = m_cellStartIndicesBuffer;
    cellIndicesBufferInfo.offset = 0;
    cellIndicesBufferInfo.range = VK_WHOLE_SIZE;

    std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
    descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[0].dstSet = m_indexingDescriptorSet;
    descriptorWrites[0].dstBinding = 0;
    descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[0].descriptorCount = 1;
    descriptorWrites[0].pBufferInfo = &hashesBufferInfo;

    descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrites[1].dstSet = m_indexingDescriptorSet;
    descriptorWrites[1].dstBinding = 1;
    descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    descriptorWrites[1].descriptorCount = 1;
    descriptorWrites[1].pBufferInfo = &cellIndicesBufferInfo;

    vkUpdateDescriptorSets(m_devices.getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void FluidSimulator::createGraphicsPipelineLayout()
{
    std::array<VkDescriptorSetLayoutBinding, 2> graphicsLayoutBindings{};
    // Binding 0: Particle Data (SSBO)
    graphicsLayoutBindings[0].binding = 0;
    graphicsLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    graphicsLayoutBindings[0].descriptorCount = 1;
    graphicsLayoutBindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    // Binding 1: Camera Data (UBO)
    graphicsLayoutBindings[1].binding = 1;
    graphicsLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    graphicsLayoutBindings[1].descriptorCount = 1;
    graphicsLayoutBindings[1].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo graphicsLayoutInfo{};
    graphicsLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    graphicsLayoutInfo.bindingCount = static_cast<uint32_t>(graphicsLayoutBindings.size());
    graphicsLayoutInfo.pBindings = graphicsLayoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_devices.getLogicalDevice(), &graphicsLayoutInfo, nullptr, &graphicsDescriptorSetLayout));

    VkPipelineLayoutCreateInfo graphicsPipelineLayoutInfo{};
    graphicsPipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    graphicsPipelineLayoutInfo.setLayoutCount = 1;
    graphicsPipelineLayoutInfo.pSetLayouts = &graphicsDescriptorSetLayout;
    VK_CHECK_RESULT(vkCreatePipelineLayout(m_devices.getLogicalDevice(), &graphicsPipelineLayoutInfo, nullptr, &graphicsPipelineLayout));
}

void FluidSimulator::createParticleBuffers()
{
    
    VkDeviceSize maxBufferSize = sizeof(Particle) * MAX_PARTICLES;
    VkDeviceSize initialDataSize = sizeof(Particle) * m_params.particleCount;
    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    VulkanBuffer::createBuffer(
        m_devices.getLogicalDevice(),
        m_devices.getPhysicalDevice(),
        initialDataSize,
        VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        stagingBuffer,
        stagingBufferMemory
    );

    void* data;
    vkMapMemory(m_devices.getLogicalDevice(), stagingBufferMemory, 0, initialDataSize, 0, &data);
    memcpy(data, m_particles.data(), (size_t)initialDataSize);
    vkUnmapMemory(m_devices.getLogicalDevice(), stagingBufferMemory);

    particleBuffers.resize(2);
    particleBufferMemories.resize(2);
    VulkanBuffer::createBuffer(
        m_devices.getLogicalDevice(),
        m_devices.getPhysicalDevice(),
        maxBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        particleBuffers[0],
        particleBufferMemories[0]
    );
    VulkanBuffer::createBuffer(
        m_devices.getLogicalDevice(),
        m_devices.getPhysicalDevice(),
        maxBufferSize,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        particleBuffers[1],
        particleBufferMemories[1]
    );

    VkCommandBuffer cmd = VulkanCommandBuffers::beginSingleTimeCommands(m_devices.getLogicalDevice(), m_commandPool);
    VkBufferCopy copyRegion{};
    copyRegion.size = initialDataSize;
    vkCmdCopyBuffer(cmd, stagingBuffer, particleBuffers[0], 1, &copyRegion);
    VulkanCommandBuffers::endSingleTimeCommands(cmd, m_devices.getLogicalDevice(), m_devices.getGraphicsQueue(), m_commandPool);

    vkDestroyBuffer(m_devices.getLogicalDevice(), stagingBuffer, nullptr);
    vkFreeMemory(m_devices.getLogicalDevice(), stagingBufferMemory, nullptr);
}

void FluidSimulator::createComputePipeline()
{
    std::array<VkDescriptorSetLayoutBinding, 5> layoutBindings{};
    // particle buffer IN
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // particle buffer OUT
    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // sorted particle hashes
    layoutBindings[2].binding = 2;
    layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[2].descriptorCount = 1;
    layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // sorted particle indices
    layoutBindings[3].binding = 3;
    layoutBindings[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[3].descriptorCount = 1;
    layoutBindings[3].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    // cell start indices
    layoutBindings[4].binding = 4;
    layoutBindings[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[4].descriptorCount = 1;
    layoutBindings[4].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = layoutBindings.size();
    layoutInfo.pBindings = layoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_devices.getLogicalDevice(), &layoutInfo, nullptr, &m_fluidComputeDescriptorSetLayout));
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_fluidComputeDescriptorSetLayout;
    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(ComputeParameters);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_devices.getLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_fluidComputePipelineLayout));

    VkShaderModule computeShaderModule = vks::tools::loadShader("shaders/fluid.comp.spv", m_devices.getLogicalDevice());

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = computeShaderModule;
    shaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = m_fluidComputePipelineLayout;
    pipelineInfo.stage = shaderStageInfo;
    VK_CHECK_RESULT(vkCreateComputePipelines(m_devices.getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_fluidComputePipeline));
    
    vkDestroyShaderModule(m_devices.getLogicalDevice(), computeShaderModule, nullptr);
}

void FluidSimulator::createComputeDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(2, m_fluidComputeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 2;
    allocInfo.pSetLayouts = layouts.data();

    m_fluidComputeDescriptorSets.resize(2);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(m_devices.getLogicalDevice(), &allocInfo, m_fluidComputeDescriptorSets.data()));

    updateComputeDescriptorSets();
}

void FluidSimulator::updateComputeDescriptorSets()
{
    for (size_t i = 0; i < 2; i++)
    {
        VkDescriptorBufferInfo bufferInfoIn{};
        bufferInfoIn.buffer = particleBuffers[i];
        bufferInfoIn.offset = 0;
        bufferInfoIn.range = sizeof(Particle) * MAX_PARTICLES;

        VkDescriptorBufferInfo bufferInfoOut{};
        bufferInfoOut.buffer = particleBuffers[(i + 1) % 2];
        bufferInfoOut.offset = 0;
        bufferInfoOut.range = sizeof(Particle) * MAX_PARTICLES;

        VkDescriptorBufferInfo sortedHashesInfo{};
        sortedHashesInfo.buffer = m_particleHashesBuffer;
        sortedHashesInfo.offset = 0;
        sortedHashesInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo sortedIndicesInfo{};
        sortedIndicesInfo.buffer = m_particleIndicesBuffer;
        sortedIndicesInfo.offset = 0;
        sortedIndicesInfo.range = VK_WHOLE_SIZE;

        VkDescriptorBufferInfo cellStartsInfo{};
        cellStartsInfo.buffer = m_cellStartIndicesBuffer;
        cellStartsInfo.offset = 0;
        cellStartsInfo.range = VK_WHOLE_SIZE;

        std::array<VkWriteDescriptorSet, 5> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = m_fluidComputeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfoIn;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = m_fluidComputeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &bufferInfoOut;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = m_fluidComputeDescriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &sortedHashesInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = m_fluidComputeDescriptorSets[i];
        descriptorWrites[3].dstBinding = 3;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &sortedIndicesInfo;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = m_fluidComputeDescriptorSets[i];
        descriptorWrites[4].dstBinding = 4;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &cellStartsInfo;

        vkUpdateDescriptorSets(m_devices.getLogicalDevice(), static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void FluidSimulator::initializeParticles()
{
    m_particles.clear();
    //m_particleVertices.clear();
    m_particles.resize(m_params.particleCount);
    //m_particleVertices.resize(m_params.particleCount);

    
    populateParticlesRandom();
    //populateParticlesSquare();
    
}

void FluidSimulator::populateParticlesRandom()
{
    float effectiveBoxWidth = m_params.boxHalfWidth - m_params.particleWorldRadius;
    float effectiveBoxHeight = m_params.boxHalfHeight - m_params.particleWorldRadius;

    for (uint32_t i = 0; i < m_params.particleCount; i++)
    {
        Particle p{};
        /*float step = (m_params.boxHalfWidth * 2) / (m_params.paricleCount + 1);
        p.position = glm::vec3(-m_params.boxHalfWidth + (step * (i + 1)), 0.0f, 0.0f);*/
        float xpos = utils::randomFloat(-effectiveBoxWidth, effectiveBoxWidth);
        float ypos = utils::randomFloat(-effectiveBoxHeight, effectiveBoxHeight);
        p.position = glm::vec4(xpos, ypos, 0.0f, 0.0f);
        p.velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
        m_particles[i] = p;

        //m_particleVertices[i].pos = p.position;

        //float t = glm::clamp(glm::length(m_particles[i].velocity) / m_params.maxSpeedForColor, 0.0f, 1.0f);
        //m_particleVertices[i].color = m_params.colorPoints.color1;
 
    }
}

void FluidSimulator::populateParticlesSquare()
{
    int particlesPerRow = static_cast<int>(std::sqrt(m_params.particleCount));
    int particlesPerCol = (m_params.particleCount - 1) / particlesPerRow + 1;
    float spacing = m_params.particleWorldRadius * 4.0f;

    for (uint32_t i = 0; i < m_params.particleCount; i++)
    {
        int col = i % particlesPerRow;
        int row = i / particlesPerRow;

        Particle p{};
        p.position.x = (col - particlesPerRow / 2.0f) * spacing;
        p.position.y = (row - particlesPerCol / 2.0f) * spacing;
        p.velocity = glm::vec4(0.0f);
        m_particles[i] = p;
        //m_particleVertices[i].pos = p.position;
        //m_particleVertices[i].color = m_params.colorPoints.color1;
    }
}



void FluidSimulator::updateContainerBuffer()
{
    m_boxVertices[0].pos = glm::vec3(-m_params.boxHalfWidth, -m_params.boxHalfHeight, 0.0f);
    m_boxVertices[1].pos = glm::vec3(m_params.boxHalfWidth, -m_params.boxHalfHeight, 0.0f);
    m_boxVertices[2].pos = glm::vec3(m_params.boxHalfWidth, m_params.boxHalfHeight, 0.0f);
    m_boxVertices[3].pos = glm::vec3(-m_params.boxHalfWidth, m_params.boxHalfHeight, 0.0f);

    void* boxdata = m_boxVertexBuffer->map();
    memcpy(boxdata, m_boxVertices.data(), m_boxVertices.size() * sizeof(Vertex));
    m_boxVertexBuffer->unmap();
}

