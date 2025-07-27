#include "VulkanCommandBuffers.h"

VulkanCommandBuffers::VulkanCommandBuffers() : commandBuffers({})
{
}

VulkanCommandBuffers::~VulkanCommandBuffers()
{
}

void VulkanCommandBuffers::create(VkDevice vkdevice, VkCommandPool commandPool, uint32_t maxFrames)
{
	//device = vkdevice;

	commandBuffers.resize(maxFrames);

	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(vkdevice, &allocInfo, commandBuffers.data()) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to allocate command buffers!");
	}
}

VkCommandBuffer VulkanCommandBuffers::beginSingleTimeCommands(VkDevice device, VkCommandPool commandPool)
{
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo{};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void VulkanCommandBuffers::endSingleTimeCommands(VkCommandBuffer commandBuffer, VkDevice device, VkQueue graphicsQueue, VkCommandPool commandPool)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo{};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

VkCommandBuffer& VulkanCommandBuffers::getCommandBuffer(uint32_t index)
{
	if (commandBuffers.size() == 0)
	{
		throw std::runtime_error("Empty command buffers!");
	}
	if (index < 0 || index >= commandBuffers.size())
	{
		throw std::runtime_error("Invalid index");
	}

	return commandBuffers[index];
}