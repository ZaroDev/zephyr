#include <pch.h>
#include "VulkanShader.h"

#include <Zephyr/FileSystem/FileSystem.h>
#include <Zephyr/FileSystem/Buffer.h>

#include "Project/Project.h"

namespace Zephyr
{
	bool LoadShaderModule(const Path& filePath, VkDevice device, VkShaderModule* outShaderModule)
	{
		const Path shaderPath = Project::GetWorkingDirectory() / filePath;
		CORE_ASSERT(FileSystem::Exists(shaderPath));
		File shaderFile = File(shaderPath, File::BINARY);
		if (!shaderFile.IsOpen())
		{
			VK_INFO("Can't open shader file {}", shaderPath.string().c_str());
			return false;
		}

		Buffer shaderData = Buffer(shaderFile.Size());
		shaderFile.Read(shaderData);

		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = shaderData.Size;
		createInfo.pCode = shaderData.As<u32>();


		VkShaderModule shaderModule;
		if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
		{
			return false;
		}

		*outShaderModule = shaderModule;

		return true;
	}
}
