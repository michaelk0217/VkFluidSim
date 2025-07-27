#include "VulkanGraphicsPipeline.h"

#include "VulkanTools.h"
#include "VulkanStructures.h"

VulkanGraphicsPipeline::VulkanGraphicsPipeline()
{
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline()
{
	vkDestroyPipeline(device, graphicsPipeline, nullptr);
}

void VulkanGraphicsPipeline::createDynamic(
	VkDevice device, 
	VkPhysicalDevice physicalDevice,
	VkPipelineLayout pipelineLayout,
	VkFormat colorAttachmentFormat,
	const std::string& vertShaderPath, 
	const std::string& fragShaderPath)
{
	this->device = device;

	// vert frag shaders
	VkShaderModule vertShaderModule = vks::tools::loadShader(vertShaderPath.c_str(), device);
	VkShaderModule fragShaderModule = vks::tools::loadShader(fragShaderPath.c_str(), device);

	VkPipelineShaderStageCreateInfo vertShaderStageCI{};
	vertShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageCI.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageCI.module = vertShaderModule;
	vertShaderStageCI.pName = "main";
	VkPipelineShaderStageCreateInfo fragShaderStageCI{};
	fragShaderStageCI.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageCI.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageCI.module = fragShaderModule;
	fragShaderStageCI.pName = "main";

	std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages = {
		vertShaderStageCI,
		fragShaderStageCI
	};

	auto bindingDescription = Vertex::getBindingDescription();
	auto attribDescription = Vertex::getAttributeDescriptions();

	VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputInfo.vertexBindingDescriptionCount = 1;
	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribDescription.size());
	vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
	vertexInputInfo.pVertexAttributeDescriptions = attribDescription.data();

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCI{};
	inputAssemblyCI.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCI.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCI.primitiveRestartEnable = VK_FALSE;

	VkPipelineViewportStateCreateInfo viewportStateCI{};
	viewportStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCI.viewportCount = 1;
	viewportStateCI.scissorCount = 1;

	VkPipelineRasterizationStateCreateInfo rasterizerStateCI{};
	rasterizerStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerStateCI.depthClampEnable = VK_FALSE;
	rasterizerStateCI.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerStateCI.lineWidth = 1.0f;
	rasterizerStateCI.cullMode = VK_CULL_MODE_NONE;
	rasterizerStateCI.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE; 
	rasterizerStateCI.depthBiasEnable = VK_FALSE;
	rasterizerStateCI.depthBiasConstantFactor = 0.0f;
	rasterizerStateCI.depthBiasClamp = 0.0f;
	rasterizerStateCI.depthBiasSlopeFactor = 0.0f;

	VkPipelineMultisampleStateCreateInfo multisamplingStateCI{};
	multisamplingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingStateCI.sampleShadingEnable = VK_FALSE;
	multisamplingStateCI.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingStateCI.minSampleShading = 1.0f;
	multisamplingStateCI.pSampleMask = nullptr;
	multisamplingStateCI.alphaToCoverageEnable = VK_FALSE;
	multisamplingStateCI.alphaToOneEnable = VK_FALSE;

	VkPipelineColorBlendAttachmentState colorBlendAttachment{};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	VkPipelineColorBlendStateCreateInfo colorBlendingStateCI{};
	colorBlendingStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingStateCI.logicOpEnable = VK_FALSE;
	colorBlendingStateCI.logicOp = VK_LOGIC_OP_COPY; // Optional;
	colorBlendingStateCI.attachmentCount = 1;
	colorBlendingStateCI.pAttachments = &colorBlendAttachment;
	colorBlendingStateCI.blendConstants[0] = 0.0f; // Optional
	colorBlendingStateCI.blendConstants[1] = 0.0f; // Optional
	colorBlendingStateCI.blendConstants[2] = 0.0f; // Optional
	colorBlendingStateCI.blendConstants[3] = 0.0f; // Optional
	
	VkPipelineDepthStencilStateCreateInfo depthStencilStateCI{};
	depthStencilStateCI.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilStateCI.depthTestEnable = VK_TRUE;
	depthStencilStateCI.depthWriteEnable = VK_TRUE;
	depthStencilStateCI.depthCompareOp = VK_COMPARE_OP_LESS;

	depthStencilStateCI.depthBoundsTestEnable = VK_FALSE;
	depthStencilStateCI.minDepthBounds = 0.0f; // Optional
	depthStencilStateCI.maxDepthBounds = 1.0f; // Optional

	depthStencilStateCI.stencilTestEnable = VK_FALSE;
	depthStencilStateCI.front = {}; // Optional
	depthStencilStateCI.back = {}; // Optional

	std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicState{};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	VkFormat depthFormat{};
	vks::tools::getSupportedDepthFormat(physicalDevice, &depthFormat);

	// dynamic rendering info
	VkPipelineRenderingCreateInfoKHR pipelineRenderingCI{ VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO_KHR };
	pipelineRenderingCI.colorAttachmentCount = 1;
	pipelineRenderingCI.pColorAttachmentFormats = &colorAttachmentFormat;
	pipelineRenderingCI.depthAttachmentFormat = depthFormat;
	pipelineRenderingCI.stencilAttachmentFormat = depthFormat;

	VkGraphicsPipelineCreateInfo pipelineCI{ VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineCI.layout = pipelineLayout;
	pipelineCI.stageCount = static_cast<uint32_t>(shaderStages.size());
	pipelineCI.pStages = shaderStages.data();
	pipelineCI.pVertexInputState = &vertexInputInfo;
	pipelineCI.pInputAssemblyState = &inputAssemblyCI;
	pipelineCI.pViewportState = &viewportStateCI;
	pipelineCI.pRasterizationState = &rasterizerStateCI;
	pipelineCI.pMultisampleState = &multisamplingStateCI;
	pipelineCI.pDepthStencilState = &depthStencilStateCI;
	pipelineCI.pColorBlendState = &colorBlendingStateCI;
	pipelineCI.pDynamicState = &dynamicState;
	pipelineCI.pNext = &pipelineRenderingCI;

	VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCI, nullptr, &graphicsPipeline));

	vkDestroyShaderModule(device, vertShaderModule, nullptr);
	vkDestroyShaderModule(device, fragShaderModule, nullptr);
}

VkPipeline VulkanGraphicsPipeline::getGraphicsPipeline() const
{
	return graphicsPipeline;
}
