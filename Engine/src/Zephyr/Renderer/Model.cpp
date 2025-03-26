#include <pch.h>
#include "Model.h"

#include <Zephyr/Renderer/Renderer.h>
namespace Zephyr
{
	Ref<Model> Model::Create(const std::array<std::vector<Mesh>, c_MaxLODCount>& meshes, u32 lodCount)
	{
		CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}
}