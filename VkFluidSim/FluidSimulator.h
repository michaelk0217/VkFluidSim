#pragma once

#include "VulkanStructures.h"
#include "VulkanDevices.h"
#include "VulkanVertexBuffer.h"
#include "VulkanIndexBuffer.h"
#include "VulkanGraphicsPipeline.h"
#include <vulkan/vulkan.h>
#include <vector>
#include <memory>


class FluidSimulator
{
public:
	
	FluidSimulator(VulkanDevices& devices, VkPipelineLayout pipelineLayout, VkCommandPool commandPool, VkFormat colorFormat);
	~FluidSimulator();

	void update(float deltaTime);
	void draw(VkCommandBuffer commandBuffer);
	
	FluidSimParameters& getParameters() { return m_params; };

private:
	void initializeParticles();
	void updateContainerBuffer();


	VulkanDevices& m_devices;

	std::vector<Particle> m_particles;
	std::vector<Vertex> m_particleVertices;
	FluidSimParameters m_params;

	// particle resources
	std::unique_ptr<VulkanGraphicsPipeline> m_particlePipeline;
	std::unique_ptr<VulkanVertexBuffer> m_particleVertexBuffer;
	const uint32_t MAX_PARTICLES = 100000;

	// container resources
	std::vector<Vertex> m_boxVertices;
	std::vector<uint32_t> m_boxIndices{
		0, 1,
		1, 2,
		2, 3,
		3, 0
	};
	std::unique_ptr<VulkanGraphicsPipeline> m_boxPipeline;
	std::unique_ptr<VulkanVertexBuffer> m_boxVertexBuffer;
	std::unique_ptr<VulkanIndexBuffer> m_boxIndexBuffer;
};

