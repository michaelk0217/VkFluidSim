#pragma once

#include <vulkan/vulkan.h>
#include <stdexcept>
#include <vector>
#include <string>
#include <set>
#include <optional>
#include <assert.h>

#include "VulkanInstance.h"
#include "VulkanSwapChain.h"

struct QueueFamilyIndices
{
	std::optional<uint32_t> graphicsFamily;
	std::optional<uint32_t> computeFamily;
	std::optional<uint32_t> transferFamily;
	std::optional<uint32_t> presentFamily;

	bool isComplete()
	{
		return graphicsFamily.has_value() && computeFamily.has_value() && transferFamily.has_value() && presentFamily.has_value();
	}
};

class VulkanDevices
{
public:
	VulkanDevices(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, const std::vector<const char*>& validationLayers);
	~VulkanDevices();

	VkDevice getLogicalDevice() const;
	VkPhysicalDevice getPhysicalDevice() const;
	VkQueue getGraphicsQueue() const;
	VkQueue getComputeQueue() const;
	VkQueue getTransferQueue() const;
	VkQueue getPresentQueue() const;
	QueueFamilyIndices getFamilyIndices() const;

	static QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);


private:
	VkPhysicalDevice physicalDevice;
	VkDevice logicalDevice;
	VkSurfaceKHR surface;
	//VkPhysicalDeviceProperties properties;
	//VkPhysicalDeviceFeatures features;
	//VkPhysicalDeviceFeatures enabledFeatures;
	//VkPhysicalDeviceMemoryProperties memoryProperties;

	VkQueue graphicsQueue;
	VkQueue computeQueue;
	VkQueue transferQueue;
	VkQueue presentQueue;

	QueueFamilyIndices indices;

	std::vector<const char*> deviceExtensions;
	std::vector<const char*> validationLayers;

	void pickPhysicalDevice(VkInstance instance);
	void createLogicalDevice();

	bool isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface);
	bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice);
	
	static uint32_t getQueueFamilyIndex(VkQueueFlags queueFlags, std::vector<VkQueueFamilyProperties> familyProperties);

};

