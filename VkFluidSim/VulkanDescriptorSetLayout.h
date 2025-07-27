#pragma once

#include <vulkan/vulkan.h>
#include <array>

class VulkanDescriptorSetLayout
{
public:
	VulkanDescriptorSetLayout(VkDevice device);
	~VulkanDescriptorSetLayout();

	VkDescriptorSetLayout getDescriptorSetLayout() const;

private:
	VkDescriptorSetLayout descriptorSetLayout{ VK_NULL_HANDLE };
	VkDevice device{ VK_NULL_HANDLE };
};

