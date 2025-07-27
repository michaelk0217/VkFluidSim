#include "VulkanIndexBuffer.h"

VulkanIndexBuffer::VulkanIndexBuffer() : device(VK_NULL_HANDLE), indexBuffer(VK_NULL_HANDLE), indexBufferMemory(VK_NULL_HANDLE)
{

}

VulkanIndexBuffer::~VulkanIndexBuffer()
{
	destroy();
}

void VulkanIndexBuffer::create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, const std::vector<uint32_t>& indices)
{
	this->device = device;

	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;

	VulkanBuffer::createBuffer(
		device,
		physicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory
	);

	void* data;
	vkMapMemory(device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	VulkanBuffer::createBuffer(
		device,
		physicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		indexBuffer,
		indexBufferMemory
	);

	VulkanBuffer::copyBuffer(
		device,
		commandPool,
		graphicsQueue,
		stagingBuffer,
		indexBuffer,
		bufferSize
	);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanIndexBuffer::destroy()
{
	if (indexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, indexBuffer, nullptr);
		indexBuffer = VK_NULL_HANDLE;
	}
	if (indexBufferMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, indexBufferMemory, nullptr);
		indexBufferMemory = VK_NULL_HANDLE;
	}
}

VkBuffer VulkanIndexBuffer::getVkBuffer() const
{
	return indexBuffer;
}