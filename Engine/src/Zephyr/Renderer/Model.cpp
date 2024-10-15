#include <pch.h>
#include "Model.h"

#include <Zephyr/Renderer/Renderer.h>
#include <Zephyr/Renderer/Platform/D3D11/D3D11Model.h>
namespace Zephyr
{
	Ref<Model> Model::Create(const std::array<std::vector<Mesh>, c_MaxLODCount>& meshes, u32 lodCount)
	{
		switch (Renderer::GetAPI())
		{
		case GraphicsAPI::OPENGL:   CORE_ASSERT(false, "RendererAPI::OPENGL is currently not supported!"); return nullptr;
		case GraphicsAPI::DX11: return CreateRef<D3D11::D3D11Model>(meshes, lodCount); 
		}
		CORE_ASSERT(false, "Unkown RendererAPI");
		return nullptr;
	}
}