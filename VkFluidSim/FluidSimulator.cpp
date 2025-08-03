#include "FluidSimulator.h"

#include "utilities.hpp"

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
        SimulationStep(deltaTime);

        for (uint32_t i = 0; i < m_particles.size(); i++)
        {

            m_particleVertices[i].pos = m_particles[i].position;
            // calculate color
            float t = glm::clamp(glm::length(m_particles[i].velocity) / m_params.maxSpeedForColor, 0.0f, 1.0f);
            //m_particleVertices[i].color = utils::lerpColor2(t, m_params.minSpeedColor, m_params.maxSpeedColor);
            std::vector<std::pair<float, glm::vec3>> colors{
                {m_params.colorPoints.c1Point, m_params.colorPoints.color1},
                {m_params.colorPoints.c2Point, m_params.colorPoints.color2},
                {m_params.colorPoints.c3Point, m_params.colorPoints.color3},
                {m_params.colorPoints.c4Point, m_params.colorPoints.color4}
            };
            m_particleVertices[i].color = utils::lerpColorVector(t, colors);
            //m_particleVertices[i].color = utils::lerpColor2(t, m_params.colorPoints.color1, m_params.colorPoints.color2);
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
    vkCmdSetLineWidth(commandBuffer, 1.0f);
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
    m_particles.resize(m_params.particleCount);
    m_particleVertices.resize(m_params.particleCount);

    
    //populateParticlesRandom();
    populateParticlesSquare();
    

    void* data = m_particleVertexBuffer->map();
    memcpy(data, m_particleVertices.data(), m_particleVertices.size() * sizeof(Vertex));
    m_particleVertexBuffer->unmap();
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
        p.position = glm::vec3(xpos, ypos, 0.0f);
        p.velocity = glm::vec3(0.0f, 0.0f, 0.0f);
        m_particles[i] = p;

        m_particleVertices[i].pos = p.position;

        float t = glm::clamp(glm::length(m_particles[i].velocity) / m_params.maxSpeedForColor, 0.0f, 1.0f);
        m_particleVertices[i].color = m_params.colorPoints.color1;
        /*std::vector<std::pair<float, glm::vec3>> colors{
                {m_params.colorPoints.c1Point, m_params.colorPoints.color1},
                {m_params.colorPoints.c2Point, m_params.colorPoints.color2},
                {m_params.colorPoints.c3Point, m_params.colorPoints.color3},
                {m_params.colorPoints.c4Point, m_params.colorPoints.color4}
        };
        m_particleVertices[i].color = utils::lerpColorVector(t, colors);*/
        //m_particleVertices[i].color = utils::lerpColor2(t, m_params.colorPoints.color1, m_params.colorPoints.color2);
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
        p.velocity = glm::vec3(0.0f);
        m_particles[i] = p;
        m_particleVertices[i].pos = p.position;
        m_particleVertices[i].color = m_params.colorPoints.color1;
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

