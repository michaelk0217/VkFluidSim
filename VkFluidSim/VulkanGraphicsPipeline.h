#pragma once

#include <vulkan/vulkan.h>

#include <vector>
#include <string>
#include <array>


class VulkanGraphicsPipeline
{
public:
	VulkanGraphicsPipeline();
	~VulkanGraphicsPipeline();

	void createDynamic(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		VkPipelineLayout pipelineLayout,
		VkFormat colorAttachmentFormat,
		const std::string& vertShaderPath,
		const std::string& fragShaderPath
	);

	VkPipeline getGraphicsPipeline() const;

private:
	VkPipeline graphicsPipeline{ VK_NULL_HANDLE };

	VkDevice device{ VK_NULL_HANDLE };
};

