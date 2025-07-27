#pragma once

#include <vulkan/vulkan.h>
#include <array>
#include <vector>

#include "VulkanTools.h"


class VulkanDescriptorPool
{
public:
	VulkanDescriptorPool(VkDevice device);
	~VulkanDescriptorPool();


	VkDescriptorPool getDescriptorPool() const;

private:
	VkDescriptorPool descriptorPool{ VK_NULL_HANDLE };
	VkDevice device{ VK_NULL_HANDLE };
};

