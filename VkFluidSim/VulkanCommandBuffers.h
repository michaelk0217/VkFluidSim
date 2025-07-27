
#pragma once
#include <vulkan/vulkan.h>
#include <vector>
#include <array>
#include <stdexcept>

//#include "Renderable.h"
//
//struct RenderableObject;

class VulkanCommandBuffers
{
public:

	VulkanCommandBuffers();
	~VulkanCommandBuffers();

	void create(VkDevice vkdevice, VkCommandPool commandPool, uint32_t maxFrames);

	static VkCommandBuffer beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool);
	static void endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool);

	VkCommandBuffer& getCommandBuffer(uint32_t index);

private:
	std::vector<VkCommandBuffer> commandBuffers;


};