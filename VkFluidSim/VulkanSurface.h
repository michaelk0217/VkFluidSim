#pragma once
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

class VulkanSurface
{
public:
	VulkanSurface(VkInstance instance, GLFWwindow* window);
	~VulkanSurface();

	VkSurfaceKHR getSurface() const;
private:
	VkInstance instance;
	VkSurfaceKHR surface;
};

