#include <pch.h>
#include "OpenGLRenderer.h"

namespace Zephyr::OpenGL
{
	bool OpenGLRenderer::Init()
	{
		CORE_INFO("D3D11: Renderer initialized!");
		return true;
	}

	void OpenGLRenderer::Shutdown()
	{
	}
	void OpenGLRenderer::OnResize(i32 width, i32 height)
	{

	}
	void OpenGLRenderer::Render()
	{
	}
}