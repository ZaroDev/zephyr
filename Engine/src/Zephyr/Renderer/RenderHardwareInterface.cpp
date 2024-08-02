#include <pch.h>
#include "RenderHardwareInterface.h"

#include <Zephyr/Renderer/Platform/D3D11/D3D11Renderer.h>
#include <Zephyr/Renderer/Platform/OpenGL/OpenGLRenderer.h>


namespace Zephyr
{
    Scope<RenderHardwareInterface> RenderHardwareInterface::Create(GraphicsAPI api)
    {
		switch (api)
		{
		case Zephyr::GraphicsAPI::OPENGL: return CreateScope<OpenGL::OpenGLRenderer>();
		case Zephyr::GraphicsAPI::DX11: return CreateScope<D3D11::D3D11Renderer>();
		}

		CORE_ASSERT(false, "Unable to create render hardware interface for an unkown API!");
		return nullptr;
    }
}
