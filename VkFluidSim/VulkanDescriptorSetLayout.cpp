#include "VulkanDescriptorSetLayout.h"

#include "VulkanTools.h"

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice device)
{
	this->device = device;

	// view proj
	VkDescriptorSetLayoutBinding frameUboLayoutBinding{};
	frameUboLayoutBinding.binding = 0;
	frameUboLayoutBinding.descriptorCount = 1;
	frameUboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	frameUboLayoutBinding.pImmutableSamplers = nullptr;
	frameUboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	std::array<VkDescriptorSetLayoutBinding, 1> bindings = {
		frameUboLayoutBinding
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo{};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();

	VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout));
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout()
{
	vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
}

VkDescriptorSetLayout VulkanDescriptorSetLayout::getDescriptorSetLayout() const
{
	return descriptorSetLayout;
}
