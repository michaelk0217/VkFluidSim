#include "VulkanPipelineLayout.h"

#include "VulkanTools.h"

VulkanPipelineLayout::VulkanPipelineLayout(VkDevice device, VkDescriptorSetLayout descriptorSetLayout, uint32_t pushConstantRangeCount, const VkPushConstantRange* pPushConstantRanges)
{
	this->device = device;
	VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = pushConstantRangeCount;
	pipelineLayoutInfo.pPushConstantRanges = pPushConstantRanges;

	VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout));
}

VulkanPipelineLayout::~VulkanPipelineLayout()
{
	if (pipelineLayout)
	{
		vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
	}
}

VkPipelineLayout VulkanPipelineLayout::getPipelineLayout() const
{
	return pipelineLayout;
}
