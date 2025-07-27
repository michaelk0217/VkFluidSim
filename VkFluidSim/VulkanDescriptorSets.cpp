#include "VulkanDescriptorSets.h"

#include "VulkanTools.h"
#include "VulkanStructures.h"

VulkanDescriptorSets::VulkanDescriptorSets()
{
}

VulkanDescriptorSets::~VulkanDescriptorSets()
{
}

void VulkanDescriptorSets::create(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t numFrames, const std::vector<VkBuffer> uniformBuffers)
{
	descriptorSets.resize(numFrames);

	std::vector<VkDescriptorSetLayout> layouts(numFrames, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(numFrames);
	allocInfo.pSetLayouts = layouts.data();

	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));

	for (size_t i = 0; i < numFrames; i++)
	{
		VkDescriptorBufferInfo frameUboInfo{};
		frameUboInfo.buffer = uniformBuffers[i];
		frameUboInfo.offset = 0;
		frameUboInfo.range = sizeof(ShaderData);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};

		// frameUbo
		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &frameUboInfo;

		vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
	}
}

std::vector<VkDescriptorSet> VulkanDescriptorSets::getDescriptorSets() const
{
	return descriptorSets;
}
