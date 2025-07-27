#pragma once

#include <vulkan/vulkan.h>

class VulkanImage
{
public:
	static void createImage(
		VkDevice device,
		VkPhysicalDevice physicalDevice,
		uint32_t width, uint32_t height,
		uint32_t mipLevels,
		uint32_t arrayLayers,
		VkFormat format,
		VkImageTiling tiling,
		VkImageUsageFlags usage,
		VkMemoryPropertyFlags properties,
		VkImage& image,
		VkDeviceMemory& imageMemory,
		VkImageCreateFlags flags = 0
	);

	static VkImageView createImageView(
		VkDevice device, 
		VkImage image, 
		VkFormat format, 
		VkImageAspectFlags aspectFlags
	);

private:
	static uint32_t findMemoryType(VkPhysicalDevice physicalDevice, uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

