#include "VulkanDescriptorSets.h"

#include "VulkanTools.h"
#include "VulkanStructures.h"

VulkanDescriptorSets::VulkanDescriptorSets()
{
}

VulkanDescriptorSets::~VulkanDescriptorSets()
{
}

void VulkanDescriptorSets::create(VkDevice device, VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, uint32_t numFrames)
{
	this->device = device;
	descriptorSets.resize(numFrames);

	std::vector<VkDescriptorSetLayout> layouts(numFrames, descriptorSetLayout);
	VkDescriptorSetAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(numFrames);
	allocInfo.pSetLayouts = layouts.data();

	VK_CHECK_RESULT(vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data()));

}

void VulkanDescriptorSets::updateGraphicsDescriptorSet(uint32_t currentFrameIndex, VkBuffer particleReadBuffer, VkBuffer uboBuffer)
{
	VkDescriptorBufferInfo particleBufferInfo{};
	particleBufferInfo.buffer = particleReadBuffer;
	particleBufferInfo.offset = 0;
	particleBufferInfo.range = sizeof(Particle) * 100000;

	VkDescriptorBufferInfo frameUboInfo{};
	frameUboInfo.buffer = uboBuffer;
	frameUboInfo.offset = 0;
	frameUboInfo.range = sizeof(ShaderData);

	std::array<VkWriteDescriptorSet, 2> descriptorWrites{};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = descriptorSets[currentFrameIndex];
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &particleBufferInfo;

	// frameUbo
	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = descriptorSets[currentFrameIndex];
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pBufferInfo = &frameUboInfo;

	vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void VulkanDescriptorSets::updateGraphicsBoxDescriptorSet(std::vector<VkBuffer> uboBuffers)
{
	for (uint32_t i = 0; i < 2; i++)
	{
		VkDescriptorBufferInfo frameUboInfo{};
		frameUboInfo.buffer = uboBuffers[i];
		frameUboInfo.offset = 0;
		frameUboInfo.range = sizeof(ShaderData);

		std::array<VkWriteDescriptorSet, 1> descriptorWrites{};
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
