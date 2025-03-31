#pragma once
#include "VulkanCommon.h"

namespace Zephyr
{
	struct VulkanPipelineBuilder final
	{
		std::vector<VkPipelineShaderStageCreateInfo> ShaderStages;

		VkPipelineInputAssemblyStateCreateInfo InputAssembly{};
		VkPipelineRasterizationStateCreateInfo Rasterizer{};
		VkPipelineColorBlendAttachmentState ColorBlendAttachment{};
		VkPipelineMultisampleStateCreateInfo Multisampling{};
		VkPipelineLayout PipelineLayout{};
		VkPipelineDepthStencilStateCreateInfo DepthStencil{};
		VkPipelineRenderingCreateInfo RenderInfo{};
		VkFormat ColorAttachmentFormat{};

		VulkanPipelineBuilder()
		{
			Clear();
		}

		void Clear();

		VkPipeline Build(VkDevice device);

		void SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
	};
}