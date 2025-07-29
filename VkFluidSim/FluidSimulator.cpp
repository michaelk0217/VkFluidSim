#include "FluidSimulator.h"

FluidSimulator::FluidSimulator(
	VulkanDevices& devices, 
	VkPipelineLayout pipelineLayout, 
    VkCommandPool commandPool,
	VkFormat colorFormat) : 
	m_devices(devices)
{

    m_boxVertices = {
        {{-m_params.boxHalfWidth, -m_params.boxHalfHeight, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{m_params.boxHalfWidth, -m_params.boxHalfHeight, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{m_params.boxHalfWidth, m_params.boxHalfHeight, 0.0f}, {1.0f, 1.0f, 1.0f}},
        {{-m_params.boxHalfWidth, m_params.boxHalfHeight, 0.0f}, {1.0f, 1.0f, 1.0f}}
    };

    m_particlePipeline = std::make_unique<VulkanGraphicsPipeline>();
    m_particlePipeline->createDynamic(
        devices.getLogicalDevice(),
        devices.getPhysicalDevice(),
        pipelineLayout,
        colorFormat,
        "shaders/particle.vert.spv",
        "shaders/particle.frag.spv"
    );
    m_particleVertexBuffer = std::make_unique<VulkanVertexBuffer>();
    m_particleVertexBuffer->createCoherent(devices.getLogicalDevice(), devices.getPhysicalDevice(), sizeof(Vertex) * MAX_PARTICLES);

    m_boxPipeline = std::make_unique <VulkanGraphicsPipeline>();
    m_boxPipeline->createLiquidBoxPipeline(
        devices.getLogicalDevice(),
        devices.getPhysicalDevice(),
        pipelineLayout,
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
}

FluidSimulator::~FluidSimulator()
{
}

void FluidSimulator::update(float deltaTime)
{
    if (m_params.resetSimulation)
    {
        initializeParticles();
        m_params.resetSimulation = false;
    }

    updateContainerBuffer();
    if (m_params.runSimulation)
    {
        glm::vec3 gravityVector(0.0f, 0.9f, 0.0f);

        for (uint32_t i = 0; i < m_particles.size(); i++)
        {
            m_particles[i].velocity += (gravityVector * deltaTime);
            m_particles[i].position += (m_particles[i].velocity * deltaTime);
            if (abs(m_particles[i].position.x) > m_params.boxHalfWidth)
            {
                m_particles[i].position.x = m_params.boxHalfWidth * glm::sign(m_particles[i].position.x);
                m_particles[i].velocity.x *= -1.0f * m_params.collisionDamping;
            }
            if (abs(m_particles[i].position.y) > m_params.boxHalfHeight)
            {
                m_particles[i].position.y = m_params.boxHalfHeight * glm::sign(m_particles[i].position.y);
                m_particles[i].velocity.y *= -1.0f * m_params.collisionDamping;
            }
            m_particleVertices[i].pos = m_particles[i].position;
        }

        void* data = m_particleVertexBuffer->map();
        memcpy(data, m_particleVertices.data(), m_particleVertices.size() * sizeof(Vertex));
        m_particleVertexBuffer->unmap();
    }
}

void FluidSimulator::draw(VkCommandBuffer commandBuffer)
{
    // bind graphics pipeline
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_particlePipeline->getGraphicsPipeline());
    // bind trainge vertex buffer (contains position and colors)
    VkDeviceSize offsets[1]{ 0 };
    VkBuffer vBuff = m_particleVertexBuffer->getVkBuffer();
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vBuff, offsets);
    // bind index buffer
    /*VkBuffer iBuff = indexBuffer->getVkBuffer();
    vkCmdBindIndexBuffer(commandBuffer, iBuff, 0, VK_INDEX_TYPE_UINT32);*/
    // draw indexed 
    //vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);
    vkCmdDraw(commandBuffer, m_particles.size(), 1, 0, 0);

    // ----- CONTAINER ------
    vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_boxPipeline->getGraphicsPipeline());
    vkCmdSetLineWidth(commandBuffer, 5.0f);
    vBuff = m_boxVertexBuffer->getVkBuffer();
    VkBuffer iBuff = m_boxIndexBuffer->getVkBuffer();
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vBuff, offsets);
    vkCmdBindIndexBuffer(commandBuffer, iBuff, 0, VK_INDEX_TYPE_UINT32);
    vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_boxIndices.size()), 1, 0, 0, 0);
}

void FluidSimulator::initializeParticles()
{
    m_particles.clear();
    m_particleVertices.clear();
    m_particles.resize(m_params.paricleCount);
    m_particleVertices.resize(m_params.paricleCount);

    for (uint32_t i = 0; i < m_params.paricleCount; i++)
    {
        Particle p{};
        float step = (m_params.boxHalfWidth * 2) / (m_params.paricleCount + 1);
        p.position = glm::vec3(-m_params.boxHalfWidth + (step * (i + 1)), 0.0f, 0.0f);
        p.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        m_particles[i] = p;

        m_particleVertices[i].pos = p.position;
        m_particleVertices[i].color = glm::vec3(0.2, 0.6, 1.0);
    }

    void* data = m_particleVertexBuffer->map();
    memcpy(data, m_particleVertices.data(), m_particleVertices.size() * sizeof(Vertex));
    m_particleVertexBuffer->unmap();
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
