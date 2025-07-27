#pragma once

#include <vulkan/vulkan.h>


class VulkanCommandPool
{
public:
	VulkanCommandPool(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	~VulkanCommandPool();

	VkCommandPool getCommandPool() const;
private:
	VkDevice device{ VK_NULL_HANDLE };
	VkCommandPool commandPool{ VK_NULL_HANDLE };
};

