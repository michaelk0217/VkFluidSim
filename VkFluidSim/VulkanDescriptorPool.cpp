#include "VulkanDescriptorPool.h"

VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device)
{
	this->device = device;

	uint32_t maxFrames = 2;

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, maxFrames}
	};

	VkDescriptorPoolCreateInfo poolCI{};
	poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCI.pPoolSizes = poolSizes.data();
	poolCI.maxSets = maxFrames;

	VK_CHECK_RESULT(vkCreateDescriptorPool(device, &poolCI, nullptr, &descriptorPool));
}

VulkanDescriptorPool::~VulkanDescriptorPool()
{
	vkDestroyDescriptorPool(device, descriptorPool, nullptr);
}

VkDescriptorPool VulkanDescriptorPool::getDescriptorPool() const
{
	return descriptorPool;
}
