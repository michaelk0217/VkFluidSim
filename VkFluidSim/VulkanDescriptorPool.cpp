#include "VulkanDescriptorPool.h"

VulkanDescriptorPool::VulkanDescriptorPool(VkDevice device)
{
	this->device = device;

	uint32_t maxFrames = 2;

	std::vector<VkDescriptorPoolSize> poolSizes = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 4}, // 2 for particle graphics + 2 for box graphics
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 20} // 2 p-graphics + 10 main-compute + 6 hash + 2 sort
	};

	VkDescriptorPoolCreateInfo poolCI{};
	poolCI.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCI.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	poolCI.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolCI.pPoolSizes = poolSizes.data();
	poolCI.maxSets = 10; // 2 p-graphics + 2 box-graphics + 2 main-compute + 2 hash + 1 sort + 1 padding

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
