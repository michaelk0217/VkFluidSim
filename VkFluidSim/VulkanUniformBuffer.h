#pragma once
#include <vulkan/vulkan.h>
#include <vector>

#include <glm/glm.hpp>

class VulkanUniformBuffers
{
public:
	VulkanUniformBuffers();
	~VulkanUniformBuffers();

	VkBuffer getBuffer(uint32_t frameIndex) const;
	
	std::vector<VkBuffer> getBuffers() const;

	void* getMappedMemory(uint32_t frameIndex) const;

	void create(VkDevice device, VkPhysicalDevice physicalDevice, uint32_t numFrames, VkDeviceSize perFrameBufferSize);

private:
	VkDevice device;
	std::vector<VkBuffer> uniformBuffers;
	std::vector<VkDeviceMemory> uniformBuffersMemory;
	std::vector<void*> uniformBuffersMapped;
	VkDeviceSize bufferSize;
	uint32_t numFrames;
};