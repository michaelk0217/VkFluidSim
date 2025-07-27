#pragma once

#include <vulkan/vulkan.h>
#include <vector>
class VulkanSyncPrimitives
{
public:
	VulkanSyncPrimitives();
	~VulkanSyncPrimitives();

	void create(VkDevice device, uint32_t numFrames, uint32_t numSwapchainImages);

	//VkSemaphore getRenderCompleteSemaphore(uint32_t frameIndex) const;
	//VkSemaphore getPresentCompleteSemaphore(uint32_t frameIndex) const;
	//VkFence getWaitFence(uint32_t frameIndex) const;

	// Fences per frame in flight
	std::vector<VkFence> waitFences;
	// Semaphores are used for correct command ordering within a queue
	// Used to ensure that image presentation is complete before starting to submit again
	std::vector<VkSemaphore> presentCompleteSemaphores;
	// Render completion
	// Semaphore used to ensure that all commands submitted have been finished 
	// before submitting the image to the queue
	std::vector<VkSemaphore> renderCompleteSemaphores;

private:
	VkDevice device;

	


};

