#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include "VulkanStructures.h"

class VulkanVertexBuffer
{
public:
	VulkanVertexBuffer();
	~VulkanVertexBuffer();

	void create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, const std::vector<Vertex>& vertices);
	void destroy();

	VkBuffer getVkBuffer() const;

	void createCoherent(VkDevice device, VkPhysicalDevice physicalDevice, VkDeviceSize size);

	void* map() const;
	void unmap() const;

private:
	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;

	VkDevice device;
};

