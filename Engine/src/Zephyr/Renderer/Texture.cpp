#include <pch.h>
#include "Texture.h"

#include <Zephyr/Renderer/Renderer.h>
#include <Zephyr/Renderer/Platform/D3D11/D3D11Texture.h>

namespace Zephyr
{
	Ref<Texture2D> Texture2D::Create(const TextureSpecification& specification, Buffer data)
	{
		switch (Renderer::GetAPI())
		{
		case GraphicsAPI::OPENGL:   CORE_ASSERT(false, "RendererAPI::None is currently not supported!"); return nullptr;
		case GraphicsAPI::DX11:  return CreateRef<D3D11::D3D11Texture2D>(specification, data);
		}

		CORE_ASSERT(false, "Unknown RendererAPI!");
		return nullptr;
	}

	

}