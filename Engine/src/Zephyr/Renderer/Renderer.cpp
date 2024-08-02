#include <pch.h>
#include "Renderer.h"

namespace Zephyr
{
	GraphicsAPI Renderer::s_API = GraphicsAPI::MAX;

	Renderer::Renderer(GraphicsAPI api)
	{
		s_API = api;
		m_GraphicsInterface = RenderHardwareInterface::Create(s_API);
	}
	Renderer::~Renderer()
	{
	}


	bool Renderer::Init()
	{
		bool ret = true;
		ret &= m_GraphicsInterface->Init();
		ret &= m_Library.LoadEngineShaders();

		return ret;
	}

	void Renderer::Shutdown()
	{
		CORE_INFO("Closing renderer!");
		m_GraphicsInterface->Shutdown();
	}
	void Renderer::OnResize(i32 width, i32 height)
	{
		CORE_INFO("Resize event: {0}x{1}p", width, height);
		m_GraphicsInterface->OnResize(width, height);
	}
	void Renderer::Render()
	{
		m_GraphicsInterface->Render();
	}
}