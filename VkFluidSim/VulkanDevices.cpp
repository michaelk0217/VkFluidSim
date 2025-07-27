#include "VulkanDevices.h"

#include "VulkanTools.h"
VulkanDevices::VulkanDevices(VkInstance instance, VkSurfaceKHR surface, const std::vector<const char*>& deviceExtensions, const std::vector<const char*>& validationLayers)
{
	this->deviceExtensions = deviceExtensions;
	this->validationLayers = validationLayers;

	this->surface = surface;

	pickPhysicalDevice(instance);
	createLogicalDevice();
}

VulkanDevices::~VulkanDevices()
{
	vkDestroyDevice(logicalDevice, nullptr);
}

VkDevice VulkanDevices::getLogicalDevice() const
{
	return logicalDevice;
}

VkPhysicalDevice VulkanDevices::getPhysicalDevice() const
{
	return physicalDevice;
}

VkQueue VulkanDevices::getGraphicsQueue() const
{
	return graphicsQueue;
}

VkQueue VulkanDevices::getComputeQueue() const
{
	return computeQueue;
}

VkQueue VulkanDevices::getTransferQueue() const
{
	return transferQueue;
}

VkQueue VulkanDevices::getPresentQueue() const
{
	return presentQueue;
}

void VulkanDevices::pickPhysicalDevice(VkInstance instance)
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	assert(deviceCount > 0);
	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	// Pick the first physical device
	for (const auto& physDevice : devices)
	{
		if (isDeviceSuitable(physDevice, surface))
		{
			this->physicalDevice = physDevice;
			break;
		}
	}
	if (physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

void VulkanDevices::createLogicalDevice()
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<uint32_t> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.computeFamily.value(), indices.transferFamily.value(), indices.presentFamily.value() };

	float queuePriority = 1.0f;

	for (uint32_t queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCI{};
		queueCI.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCI.queueFamilyIndex = queueFamily;
		queueCI.queueCount = 1;
		queueCI.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCI);
	}

	VkPhysicalDeviceFeatures deviceFeatures{};
	deviceFeatures.samplerAnisotropy = VK_TRUE;
	deviceFeatures.tessellationShader = VK_TRUE;
	deviceFeatures.fillModeNonSolid = VK_TRUE;

	VkPhysicalDeviceDynamicRenderingFeaturesKHR dynamic_rendering_feature{};
	dynamic_rendering_feature.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES_KHR;
	dynamic_rendering_feature.dynamicRendering = VK_TRUE;

	VkPhysicalDeviceVulkan13Features vk13Features{ VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
	vk13Features.dynamicRendering = VK_TRUE;
	vk13Features.synchronization2 = VK_TRUE;

	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();
	//createInfo.pNext = &dynamic_rendering_feature;
	createInfo.pNext = &vk13Features;

	if (enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}

	VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &createInfo, nullptr, &logicalDevice));

	vkGetDeviceQueue(logicalDevice, indices.graphicsFamily.value(), 0, &graphicsQueue);
	vkGetDeviceQueue(logicalDevice, indices.computeFamily.value(), 0, &computeQueue);
	vkGetDeviceQueue(logicalDevice, indices.transferFamily.value(), 0, &transferQueue);
	vkGetDeviceQueue(logicalDevice, indices.presentFamily.value(), 0, &presentQueue);
}

QueueFamilyIndices VulkanDevices::findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
    QueueFamilyIndices indices;

    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

	indices.graphicsFamily = getQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamilies);
	indices.computeFamily = getQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamilies);
	indices.transferFamily = getQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamilies);
	for (uint32_t i = 0; i < queueFamilyCount; i++)
	{
		VkBool32 presentSupport = VK_FALSE;
		vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
		if (presentSupport)
		{
			indices.presentFamily = i;
			// Prefer a queue family that supports both graphics and presentation
			if (indices.graphicsFamily.has_value() && indices.graphicsFamily.value() == i)
			{
				break;
			}
		}
	}

	return indices;
}


/**
* Get the index of a queue family that supports the requested queue flags
* SRS - support VkQueueFlags parameter for requesting multiple flags vs. VkQueueFlagBits for a single flag only
*
* @param queueFlags Queue flags to find a queue family index for
*
* @return Index of the queue family index that matches the flags
*
* @throw Throws an exception if no queue family index could be found that supports the requested flags
*/
uint32_t VulkanDevices::getQueueFamilyIndex(VkQueueFlags queueFlags, std::vector<VkQueueFamilyProperties> familyProperties)
{
	// Dedicated queue for compute
	// Try to find a queue family index that supports compute but not graphics
	if ((queueFlags & VK_QUEUE_COMPUTE_BIT) == queueFlags) // checks if queueFlags has exactly the compute bit set
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(familyProperties.size()); i++)
		{
			if ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) && ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
			{
				return i;
			}
		}
	}

	// Dedicated queue for transfer
	// Try to find a queue family index that supports transfer but not graphics and compute
	if ((queueFlags & VK_QUEUE_TRANSFER_BIT) == queueFlags)
	{
		for (uint32_t i = 0; i < static_cast<uint32_t>(familyProperties.size()); i++)
		{
			if ((familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT) && ((familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) && ((familyProperties[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
			{
				return i;
			}
		}
	}

	// For other queue types or if no separate compute queue is present, reutrn the first one to support the requested flags
	for (uint32_t i = 0; i < static_cast<uint32_t>(familyProperties.size()); i++)
	{
		if ((familyProperties[i].queueFlags & queueFlags) == queueFlags)
		{
			return i;
		}
	}

	throw std::runtime_error("Could not find a matching queue family index");
}

bool VulkanDevices::isDeviceSuitable(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface)
{
	QueueFamilyIndices indices = findQueueFamilies(physicalDevice, surface);

	bool extensionsSupported = checkDeviceExtensionSupport(physicalDevice);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport = VulkanSwapChain::querySwapChainSupport(physicalDevice, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	VkPhysicalDeviceFeatures supportedFeatures;
	vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

	return indices.isComplete() && extensionsSupported && swapChainAdequate && supportedFeatures.samplerAnisotropy && supportedFeatures.fillModeNonSolid;
}

bool VulkanDevices::checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}


