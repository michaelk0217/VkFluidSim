#include "VulkanDepthResources.h"

#include "VulkanImage.h"
#include "VulkanTools.h"
#include "VulkanCommandBuffers.h"

VulkanDepthResources::VulkanDepthResources()
{
	
}



VulkanDepthResources::~VulkanDepthResources()
{
	cleanup();
}

void VulkanDepthResources::create(VkDevice device, VkPhysicalDevice physicalDevice, VkQueue graphicsQueue, VkCommandPool commandPool, VkExtent2D swapChainExtent)
{
	this->device = device;
	VkFormat depthFormat{};
	vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);

	VulkanImage::createImage(
		device,
		physicalDevice,
		swapChainExtent.width,
		swapChainExtent.height,
		1, 1,
		depthFormat,
		VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		depthImage,
		depthImageMemory
	);
	depthImageView = VulkanImage::createImageView(device, depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);


	VkImageSubresourceRange subrsc{};
	subrsc.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
	subrsc.baseMipLevel = 0;
	subrsc.layerCount = 1;
	subrsc.levelCount = 1;

	if (vks::tools::formatHasStencil(depthFormat))
	{
		subrsc.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}

	VkCommandBuffer cmd = VulkanCommandBuffers::beginSingleTimeCommands(device, commandPool);
	vks::tools::setImageLayout(cmd, depthImage, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, subrsc, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT);
	VulkanCommandBuffers::endSingleTimeCommands(cmd, device, graphicsQueue, commandPool);
}

void VulkanDepthResources::cleanup()
{
	if (depthImage != VK_NULL_HANDLE)
	{
		vkDestroyImage(device, depthImage, nullptr);
		depthImage = VK_NULL_HANDLE;
	}
	if (depthImageView != VK_NULL_HANDLE)
	{
		vkDestroyImageView(device, depthImageView, nullptr);
		depthImageView = VK_NULL_HANDLE;
	}
	if (depthImageMemory != VK_NULL_HANDLE)
	{
		vkFreeMemory(device, depthImageMemory, nullptr);
		depthImageMemory = VK_NULL_HANDLE;
	}
}

VkImage VulkanDepthResources::getImage() const
{
	return depthImage;
}

VkImageView VulkanDepthResources::getDepthImageView() const
{
	return depthImageView;
}
