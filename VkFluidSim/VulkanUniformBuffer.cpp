#include "VulkanUniformBuffer.h"

#include "VulkanBuffer.h"

VulkanUniformBuffers::VulkanUniformBuffers()
{
}

VulkanUniformBuffers::~VulkanUniformBuffers()
{
	if (!uniformBuffers.empty())
	{
		for (size_t i = 0; i < numFrames; i++)
		{
			if (uniformBuffers[i] != VK_NULL_HANDLE)
			{
				vkDestroyBuffer(device, uniformBuffers[i], nullptr);
				uniformBuffers[i] = VK_NULL_HANDLE;
			}
		}
		uniformBuffers.clear();
	}

	if (!uniformBuffersMemory.empty())
	{
		for (size_t i = 0; i < numFrames; i++)
		{
			if (uniformBuffersMemory[i] != VK_NULL_HANDLE)
			{
				vkFreeMemory(device, uniformBuffersMemory[i], nullptr);
				uniformBuffersMemory[i] = VK_NULL_HANDLE;
			}
		}
		uniformBuffersMemory.clear();
	}
	uniformBuffersMapped.clear();
}

VkBuffer VulkanUniformBuffers::getBuffer(uint32_t frameIndex) const
{
	return uniformBuffers[frameIndex];
}

std::vector<VkBuffer> VulkanUniformBuffers::getBuffers() const
{
	return uniformBuffers;
}

void* VulkanUniformBuffers::getMappedMemory(uint32_t frameIndex) const
{
	return uniformBuffersMapped[frameIndex];
}

void VulkanUniformBuffers::create(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t numFrames, VkDeviceSize perFrameBufferSize)
{
	this->device = device;
	this->numFrames = numFrames;
	this->bufferSize = perFrameBufferSize;

	uniformBuffers.resize(numFrames);
	uniformBuffersMemory.resize(numFrames);
	uniformBuffersMapped.resize(numFrames);

	for (size_t i = 0; i < numFrames; i++)
	{
		VulkanBuffer::createBuffer(
			device, physicalDevice, bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniformBuffers[i],
			uniformBuffersMemory[i]
		);
		vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
	}
}
