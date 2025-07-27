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
		uint32_t numFrames,
		const std::vector<VkBuffer> uniformBuffers);

	std::vector<VkDescriptorSet> getDescriptorSets() const;
	
private:
	VkDevice device;
	std::vector<VkDescriptorSet> descriptorSets;
};

