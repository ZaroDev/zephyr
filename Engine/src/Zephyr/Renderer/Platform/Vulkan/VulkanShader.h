#pragma once
#include "VulkanCommon.h"

namespace Zephyr
{
	bool LoadShaderModule(const Path& filePath, VkDevice device, VkShaderModule* outShaderModule);
}