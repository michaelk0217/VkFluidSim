#include "VulkanVertexBuffer.h"

#include "VulkanBuffer.h"

VulkanVertexBuffer::VulkanVertexBuffer()
{
}

VulkanVertexBuffer::~VulkanVertexBuffer()
{
	destroy();
}

void VulkanVertexBuffer::create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, const std::vector<Vertex>& vertices)
{
	this->device = device;

	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

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
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(device, stagingBufferMemory);

	VulkanBuffer::createBuffer(
		device,
		physicalDevice,
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		vertexBuffer,
		vertexBufferMemory
	);

	VulkanBuffer::copyBuffer(
		device,
		commandPool,
		graphicsQueue,
		stagingBuffer,
		vertexBuffer,
		bufferSize
	);

	vkDestroyBuffer(device, stagingBuffer, nullptr);
	vkFreeMemory(device, stagingBufferMemory, nullptr);
}

void VulkanVertexBuffer::destroy()
{
	if (vertexBuffer != VK_NULL_HANDLE)
	{
		vkDestroyBuffer(device, vertexBuffer, nullptr);
		vertexBuffer = VK_NULL_HANDLE;
	}
	if (vertexBufferMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, vertexBufferMemory, nullptr);
		vertexBufferMemory = VK_NULL_HANDLE;
	}
}

VkBuffer VulkanVertexBuffer::getVkBuffer() const
{
	return vertexBuffer;
}
