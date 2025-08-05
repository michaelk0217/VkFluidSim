#pragma once

#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanUniformBuffer.h"

class VulkanDescriptorSets
{
public:
	VulkanDescriptorSets();
	~VulkanDescriptorSets();

	void create(
		VkDevice device,
		VkDescriptorPool descriptorPool,
		VkDescriptorSetLayout descriptorSetLayout,
		uint32_t numFrames
	);

	void updateGraphicsDescriptorSet(uint32_t currentFrameIndex, VkBuffer particleReadBuffer, VkBuffer uboBuffer);

	void updateGraphicsBoxDescriptorSet(std::vector<VkBuffer> uboBuffers);

	std::vector<VkDescriptorSet> getDescriptorSets() const;
	
private:
	VkDevice device;
	std::vector<VkDescriptorSet> descriptorSets;
};

