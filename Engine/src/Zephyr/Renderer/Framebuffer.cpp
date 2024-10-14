#include <pch.h>
#include "Framebuffer.h"

#include <Zephyr/Renderer/Renderer.h>

#include <Zephyr/Renderer/Platform/D3D11/D3D11Framebuffer.h>


namespace Zephyr
{
	Ref<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec)
	{
		switch (Renderer::GetAPI())
		{
		case GraphicsAPI::OPENGL: CORE_ASSERT(false); return nullptr;
		case GraphicsAPI::DX11: return CreateRef<D3D11::D3D11Framebuffer>(spec);
		}

		return nullptr;
	}
}