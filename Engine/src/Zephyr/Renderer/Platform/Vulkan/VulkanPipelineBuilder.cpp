#include <pch.h>
#include "VulkanPipelineBuilder.h"

#include "VulkanUtils.h"

namespace Zephyr
{
	void VulkanPipelineBuilder::Clear()
	{
		InputAssembly = {};
		InputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

		Rasterizer = {};
		Rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

		ColorBlendAttachment = {};

		Multisampling = {};
		Multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

		PipelineLayout = {};

		DepthStencil = {};
		DepthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

		RenderInfo = {};
		RenderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

		ShaderStages.clear();
	}

	VkPipeline VulkanPipelineBuilder::Build(VkDevice device)
	{
		// Make viewport state from our stored viewport and scissor.
		// At the moment we won't support multiple viewports or scissors.
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.pNext = nullptr;
		viewportState.viewportCount = 1;
		viewportState.scissorCount = 1;

		// Setup dummy color blending. We aren't using transparent objects yet
		// The blending is just "no blend", but we do write to the color attachment
		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.pNext = nullptr;
		colorBlending.logicOpEnable = VK_FALSE;
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &ColorBlendAttachment;

		// Completely clear vertex input state create info, as we have no need for it
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

		// Build the pipeline
		// We now use all the info structs we have been writing into this one to create the pipeline
		VkGraphicsPipelineCreateInfo pipelineCreateInfo{};
		pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineCreateInfo.pNext = &RenderInfo;
		pipelineCreateInfo.stageCount = static_cast<u32>(ShaderStages.size());
		pipelineCreateInfo.pStages = ShaderStages.data();
		pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
		pipelineCreateInfo.pInputAssemblyState = &InputAssembly;
		pipelineCreateInfo.pViewportState = &viewportState;
		pipelineCreateInfo.pRasterizationState = &Rasterizer;
		pipelineCreateInfo.pMultisampleState = &Multisampling;
		pipelineCreateInfo.pColorBlendState = &colorBlending;
		pipelineCreateInfo.pDepthStencilState = &DepthStencil;
		pipelineCreateInfo.layout = PipelineLayout;

		VkDynamicState states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
		VkPipelineDynamicStateCreateInfo dynamicInfo{};
		dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicInfo.pDynamicStates = &states[0];
		dynamicInfo.dynamicStateCount = 2;

		pipelineCreateInfo.pDynamicState = &dynamicInfo;

		VkPipeline newPipeline;
		if(vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &newPipeline) != VK_SUCCESS)
		{
			VK_ERROR("Failed to create pipeline");
			return VK_NULL_HANDLE;
		}

		return newPipeline;
	}

	void VulkanPipelineBuilder::SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader)
	{
		ShaderStages.clear();

		ShaderStages.push_back(Utils::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader));
		ShaderStages.push_back(Utils::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader));
	}
	void VulkanPipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
	{
		InputAssembly.topology = topology;
		// We are not going to use primitive restart for the moment
		InputAssembly.primitiveRestartEnable = VK_FALSE;
	}
	void VulkanPipelineBuilder::SetPolygonMode(VkPolygonMode polygonMode)
	{
		Rasterizer.polygonMode = polygonMode;
		Rasterizer.lineWidth = 1.0f;
	}
	void VulkanPipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
	{
		Rasterizer.cullMode = cullMode;
		Rasterizer.frontFace = frontFace;
	}
	void VulkanPipelineBuilder::SetMultisamplingNone()
	{
		Multisampling.sampleShadingEnable = VK_FALSE;
		// No multisampling means 1 sample per pixel
		Multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		Multisampling.minSampleShading = 1.0f;
		Multisampling.pSampleMask = nullptr;
		// No alpha coverage either
		Multisampling.alphaToCoverageEnable = VK_FALSE;
		Multisampling.alphaToOneEnable = VK_FALSE;
	}
	void VulkanPipelineBuilder::DisableBlending()
	{
		ColorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT;
		ColorBlendAttachment.blendEnable = VK_FALSE;
	}
	void VulkanPipelineBuilder::SetColorAttachmentFormat(VkFormat format)
	{
		ColorAttachmentFormat = format;
		RenderInfo.colorAttachmentCount = 1;
		RenderInfo.pColorAttachmentFormats = &ColorAttachmentFormat;
	}
	void VulkanPipelineBuilder::SetDepthFormat(VkFormat format)
	{
		RenderInfo.depthAttachmentFormat = format;
	}
	void VulkanPipelineBuilder::DisableDepthTest()
	{
		DepthStencil.depthTestEnable = VK_FALSE;
		DepthStencil.depthWriteEnable = VK_FALSE;
		DepthStencil.depthCompareOp = VK_COMPARE_OP_NEVER;
		DepthStencil.depthBoundsTestEnable = VK_FALSE;
		DepthStencil.stencilTestEnable = VK_FALSE;
		DepthStencil.front = {};
		DepthStencil.back = {};
		DepthStencil.minDepthBounds = 0.0f;
		DepthStencil.maxDepthBounds = 1.0f;
	}
}
