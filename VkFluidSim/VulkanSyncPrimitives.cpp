#include "VulkanSyncPrimitives.h"

#include <stdexcept>

#include "VulkanTools.h"
#include "VulkanInitializers.hpp"

VulkanSyncPrimitives::VulkanSyncPrimitives()
{
}

VulkanSyncPrimitives::~VulkanSyncPrimitives()
{
	if (!presentCompleteSemaphores.empty())
	{
		for (auto& semaphore : presentCompleteSemaphores)
		{
			vkDestroySemaphore(device, semaphore, nullptr);
		}
		presentCompleteSemaphores.clear();
	}

	if (!renderCompleteSemaphores.empty())
	{
		for (auto& semaphore : renderCompleteSemaphores)
		{
			vkDestroySemaphore(device, semaphore, nullptr);
		}
		renderCompleteSemaphores.clear();
	}

	if (!waitFences.empty())
	{
		for (auto& fence : waitFences)
		{
			vkDestroyFence(device, fence, nullptr);
		}
		waitFences.clear();
	}
}

void VulkanSyncPrimitives::create(VkDevice device, uint32_t numFrames, uint32_t numSwapchainImages)
{
	this->device = device;

	waitFences.resize(numFrames);
	renderCompleteSemaphores.resize(numSwapchainImages);
	presentCompleteSemaphores.resize(numFrames);

	for (uint32_t i = 0; i < numFrames; i++)
	{
		VkFenceCreateInfo fenceCI = vks::initializers::fenceCreateInfo(VK_FENCE_CREATE_SIGNALED_BIT);
		VK_CHECK_RESULT(vkCreateFence(device, &fenceCI, nullptr, &waitFences[i]));
	}

	for (uint32_t i = 0; i < numFrames; i++)
	{
		VkSemaphoreCreateInfo semaphoreCI = vks::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &presentCompleteSemaphores[i]));
	}

	for (uint32_t i = 0; i < numSwapchainImages; i++)
	{
		VkSemaphoreCreateInfo semaphoreCI = vks::initializers::semaphoreCreateInfo();
		VK_CHECK_RESULT(vkCreateSemaphore(device, &semaphoreCI, nullptr, &renderCompleteSemaphores[i]));
	}
}

//VkSemaphore VulkanSyncPrimitives::getRenderCompleteSemaphore(uint32_t frameIndex) const
//{
//	if (frameIndex < 0 || frameIndex >= renderCompleteSemaphores.size())
//	{
//		throw std::runtime_error("Invalid frame index for renderCompleteSemaphores");
//	}
//	return renderCompleteSemaphores[frameIndex];
//}
//
//VkSemaphore VulkanSyncPrimitives::getPresentCompleteSemaphore(uint32_t index) const
//{
//	if (index < 0 || index >= presentCompleteSemaphores.size())
//	{
//		throw std::runtime_error("Invalid index for presentCompleteSemaphores");
//	}
//	return presentCompleteSemaphores[index];
//}
//
//VkFence VulkanSyncPrimitives::getWaitFence(uint32_t frameIndex) const
//{
//	if (frameIndex < 0 || frameIndex >= waitFences.size())
//	{
//		throw std::runtime_error("Invalid frame index for waitFences");
//	}
//	return waitFences[frameIndex];
//}
