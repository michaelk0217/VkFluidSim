#pragma once

#include <vulkan/vulkan.h>
#include <vector>

class VulkanDepthResources
{
public:
	VulkanDepthResources();
	~VulkanDepthResources();

	void create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkExtent2D swapChainExtent);
	void cleanup();

	VkImage getImage() const;

	VkImageView getDepthImageView() const;

private:
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkDevice device;
};

