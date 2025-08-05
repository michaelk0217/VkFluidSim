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
    if (computePipeline != VK_NULL_HANDLE)
    {
        vkDestroyPipeline(m_devices.getLogicalDevice(), computePipeline, nullptr);
    }
    if (computePipelineLayout != VK_NULL_HANDLE)
    {
        vkDestroyPipelineLayout(m_devices.getLogicalDevice(), computePipelineLayout, nullptr);
    }
    if (computeDescriptorSetLayout != VK_NULL_HANDLE)
    {
        vkDestroyDescriptorSetLayout(m_devices.getLogicalDevice(), computeDescriptorSetLayout, nullptr);
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
}

void FluidSimulator::update(float deltaTime, VkCommandBuffer commandBuffer)
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
        updateComputeDescriptorSets();
        currentBuffer = 0;

        m_params.resetSimulation = false;
    }

    updateContainerBuffer();

    if (m_params.runSimulation || did_reset)
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_COMPUTE,
            computePipelineLayout,
            0, 1,
            &computeDescriptorSets[currentBuffer],
            0, nullptr
        );

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
        push.color1 = glm::vec4(m_params.colorPoints.color1, 1.0f);
        push.color2 = glm::vec4(m_params.colorPoints.color2, 1.0f);
        push.color3 = glm::vec4(m_params.colorPoints.color3, 1.0f);
        push.maxSpeedForColor = m_params.maxSpeedForColor;

        vkCmdPushConstants(commandBuffer, computePipelineLayout, VK_SHADER_STAGE_COMPUTE_BIT, 0, sizeof(ComputeParameters), &push);

        uint32_t groupCountX = (m_params.particleCount + 255) / 256;
        vkCmdDispatch(commandBuffer, groupCountX, 1, 1);

        currentBuffer = 1 - currentBuffer;

        //SimulationStep(deltaTime);

        //for (uint32_t i = 0; i < m_particles.size(); i++)
        //{

        //    m_particleVertices[i].pos = m_particles[i].position;
        //    // calculate color
        //    float t = glm::clamp(glm::length(m_particles[i].velocity) / m_params.maxSpeedForColor, 0.0f, 1.0f);
        //    //m_particleVertices[i].color = utils::lerpColor2(t, m_params.minSpeedColor, m_params.maxSpeedColor);
        //    std::vector<std::pair<float, glm::vec3>> colors{
        //        {m_params.colorPoints.c1Point, m_params.colorPoints.color1},
        //        {m_params.colorPoints.c2Point, m_params.colorPoints.color2},
        //        {m_params.colorPoints.c3Point, m_params.colorPoints.color3},
        //        {m_params.colorPoints.c4Point, m_params.colorPoints.color4}
        //    };
        //    m_particleVertices[i].color = utils::lerpColorVector(t, colors);
        //    //m_particleVertices[i].color = utils::lerpColor2(t, m_params.colorPoints.color1, m_params.colorPoints.color2);
        //}

        //void* data = m_particleVertexBuffer->map();
        //memcpy(data, m_particleVertices.data(), m_particleVertices.size() * sizeof(Vertex));
        //m_particleVertexBuffer->unmap();
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
    std::array<VkDescriptorSetLayoutBinding, 2> layoutBindings{};
    layoutBindings[0].binding = 0;
    layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[0].descriptorCount = 1;
    layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    layoutBindings[1].binding = 1;
    layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    layoutBindings[1].descriptorCount = 1;
    layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = layoutBindings.size();
    layoutInfo.pBindings = layoutBindings.data();
    VK_CHECK_RESULT(vkCreateDescriptorSetLayout(m_devices.getLogicalDevice(), &layoutInfo, nullptr, &computeDescriptorSetLayout));
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;
    VkPushConstantRange pushConstant{};
    pushConstant.offset = 0;
    pushConstant.size = sizeof(ComputeParameters);
    pushConstant.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;
    pipelineLayoutInfo.pushConstantRangeCount = 1;

    VK_CHECK_RESULT(vkCreatePipelineLayout(m_devices.getLogicalDevice(), &pipelineLayoutInfo, nullptr, &computePipelineLayout));

    VkShaderModule computeShaderModule = vks::tools::loadShader("shaders/fluid.comp.spv", m_devices.getLogicalDevice());

    VkPipelineShaderStageCreateInfo shaderStageInfo{};
    shaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    shaderStageInfo.module = computeShaderModule;
    shaderStageInfo.pName = "main";

    VkComputePipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    pipelineInfo.layout = computePipelineLayout;
    pipelineInfo.stage = shaderStageInfo;
    VK_CHECK_RESULT(vkCreateComputePipelines(m_devices.getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline));
    
    vkDestroyShaderModule(m_devices.getLogicalDevice(), computeShaderModule, nullptr);
}

void FluidSimulator::createComputeDescriptorSets()
{
    std::vector<VkDescriptorSetLayout> layouts(2, computeDescriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = 2;
    allocInfo.pSetLayouts = layouts.data();

    computeDescriptorSets.resize(2);
    VK_CHECK_RESULT(vkAllocateDescriptorSets(m_devices.getLogicalDevice(), &allocInfo, computeDescriptorSets.data()));

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

        std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = computeDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfoIn;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = computeDescriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &bufferInfoOut;

        vkUpdateDescriptorSets(m_devices.getLogicalDevice(), descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
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

