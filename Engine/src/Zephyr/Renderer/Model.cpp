#include <pch.h>
#include "Model.h"

#include <Zephyr/Renderer/Renderer.h>

#include "Platform/Vulkan/VulkanModel.h"
namespace Zephyr
{
	Ref<Model> Model::Create(const std::array<std::vector<Mesh>, c_MaxLODCount>& meshes, u32 lodCount)
	{
		return CreateRef<VulkanModel>(meshes, lodCount);
	}
}