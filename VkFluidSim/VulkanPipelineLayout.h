#pragma once

#include <vulkan/vulkan.h>

class VulkanPipelineLayout
{
public:
	VulkanPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, uint32_t pushConstantRangeCount = 0, const VkPushConstantRange* pPushConstantRanges = nullptr);
	~VulkanPipelineLayout();
	
	VkPipelineLayout getPipelineLayout() const;

private:
	VkDevice device;
	VkPipelineLayout pipelineLayout;
};

