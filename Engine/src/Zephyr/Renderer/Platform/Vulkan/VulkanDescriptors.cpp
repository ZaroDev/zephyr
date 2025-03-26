#include <pch.h>
#include "VulkanDescriptors.h"


namespace Zephyr
{
	void DescriptorLayoutBuilder::AddBinding(u32 binding, VkDescriptorType type)
	{
		VkDescriptorSetLayoutBinding newBinding{};
		newBinding.binding = binding;
		newBinding.descriptorCount = 1;
		newBinding.descriptorType = type;

		Bindings.push_back(newBinding);
	}

	void DescriptorLayoutBuilder::Clear()
	{
		Bindings.clear();
	}

	VkDescriptorSetLayout DescriptorLayoutBuilder::Build(VkDevice device, VkShaderStageFlags shaderStages, void* pNext,
		VkDescriptorSetLayoutCreateFlags flags)
	{
		for (auto& binding : Bindings)
		{
			binding.stageFlags |= shaderStages;
		}

		VkDescriptorSetLayoutCreateInfo info{};
		info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		info.pBindings = Bindings.data();
		info.bindingCount = static_cast<u32>(Bindings.size());
		info.flags = flags;

		VkDescriptorSetLayout set;
		VK_ASSERT(vkCreateDescriptorSetLayout(device, &info, nullptr, &set));

		return set;
	}

	void DescriptorAllocator::InitPool(VkDevice device, u32 maxSets, std::span<PoolSizeRatio> poolRatios)
	{
		std::vector<VkDescriptorPoolSize> poolSizes;
		for (PoolSizeRatio& ratio : poolRatios)
		{
			poolSizes.push_back(VkDescriptorPoolSize{
				.type = ratio.Type,
				.descriptorCount = static_cast<u32>(ratio.Ratio * maxSets)
				}
			);
		}

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.maxSets = maxSets;
		poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
		poolInfo.pPoolSizes = poolSizes.data();

		vkCreateDescriptorPool(device, &poolInfo, nullptr, &Pool);
	}

	void DescriptorAllocator::ClearDescriptors(VkDevice device)
	{
		vkResetDescriptorPool(device, Pool, 0);
	}

	void DescriptorAllocator::DestroyPool(VkDevice device)
	{
		vkDestroyDescriptorPool(device, Pool, nullptr);
	}

	VkDescriptorSet DescriptorAllocator::Allocate(VkDevice device, VkDescriptorSetLayout layout)
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = Pool;
		allocInfo.descriptorSetCount = 1;
		allocInfo.pSetLayouts = &layout;

		VkDescriptorSet descriptorSet;
		VK_ASSERT(vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet));
		return descriptorSet;
	}
}
