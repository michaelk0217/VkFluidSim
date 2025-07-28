#pragma once

#include <vulkan/vulkan.h>
#include <memory>

#include "VulkanStructures.h"

class Window;
class VulkanInstance;
class VulkanDevices;
class VulkanSurface;
class VulkanSwapChain;
class VulkanCommandPool;

class ImGuiManager
{
public:
	ImGuiManager(
		Window& window,
		VulkanInstance& instance,
		VulkanDevices& devices,
		VulkanSurface& surface,
		VulkanSwapChain& swapChain,
		VulkanCommandPool& commandPool
	);
	~ImGuiManager();

	ImGuiManager(const ImGuiManager&) = delete;
	ImGuiManager& operator=(const ImGuiManager&) = delete;

	void newFrame();
	void buildUI(UiContextPacket& uiPacket);
	void render(VkCommandBuffer commandBuffer, uint32_t imageIndex, uint32_t width, uint32_t height);

	//void check_vk_result(VkResult err);
private:
	VulkanDevices& m_devices;
	VulkanSwapChain& m_swapChain;
	VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;

	
};

