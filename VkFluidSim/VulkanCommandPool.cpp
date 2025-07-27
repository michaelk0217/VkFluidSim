#include "VulkanCommandPool.h"


#include "VulkanDevices.h"
#include "VulkanTools.h"

VulkanCommandPool::VulkanCommandPool(VkDevice device, VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	this->device = device;
	QueueFamilyIndices queueFamilyIndices = VulkanDevices::findQueueFamilies(physicalDevice, surface);
	
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

	VK_CHECK_RESULT(vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool));
}

VulkanCommandPool::~VulkanCommandPool()
{
	if (commandPool != VK_NULL_HANDLE)
	{
		vkDestroyCommandPool(device, commandPool, nullptr);
	}
}

VkCommandPool VulkanCommandPool::getCommandPool() const
{
	return commandPool;
}
