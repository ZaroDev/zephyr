#pragma once
#include <span>
#include "VulkanCommon.h"

namespace Zephyr
{
	struct DescriptorLayoutBuilder final
	{
		std::vector<VkDescriptorSetLayoutBinding> Bindings;

		void AddBinding(u32 binding, VkDescriptorType type);
		void Clear();

		VkDescriptorSetLayout Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext = nullptr, VkDescriptorSetLayoutCreateFlags flags = 0);
	};

	struct DescriptorAllocator final
	{
		struct PoolSizeRatio
		{
			VkDescriptorType Type;
			f32 Ratio;
		};

		VkDescriptorPool Pool;

		void InitPool(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios);
		void ClearDescriptors(VkDevice device);
		void DestroyPool(VkDevice device);

		VkDescriptorSet Allocate(VkDevice device, VkDescriptorSetLayout layout);
	};
}
 