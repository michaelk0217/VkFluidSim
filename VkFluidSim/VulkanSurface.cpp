#include "VulkanSurface.h"
#include "VulkanTools.h"

VulkanSurface::VulkanSurface(VkInstance instance, GLFWwindow* window)
{
	this->instance = instance;
	VK_CHECK_RESULT(glfwCreateWindowSurface(instance, window, nullptr, &surface));
}

VulkanSurface::~VulkanSurface()
{
	vkDestroySurfaceKHR(instance, surface, nullptr);
}

VkSurfaceKHR VulkanSurface::getSurface() const
{
	return surface;
}
