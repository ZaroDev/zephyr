#include <pch.h>
#include "Application.h"


namespace Zephyr
{
	Application* Application::s_Instance = nullptr;

	Application::Application(const ApplicationSpecification& specs)
	{
		CORE_ASSERT(!s_Instance, "Application already initialized");
		CORE_INFO("Initializing application!");
		
		s_Instance = this;
		m_Running = true;
		m_Specification = specs;

		const auto windowData = WindowData
		{
			.Width = 1280,
			.Height = 720,
			.Title = m_Specification.Name,

			.Fullscreen = false,
			.Vsync = false,
			.API = m_Specification.API
		};

		m_Window = CreateScope<Window>(windowData);
		if (!m_Window->Init())
		{
			CORE_CRITICAL("Unable to initialize window!");
			CORE_ASSERT(false);
			return;
		}

		m_Renderer = CreateScope<Renderer>(m_Specification.API);
		if (!m_Renderer->Init())
		{
			CORE_CRITICAL("Unable to initialize renderer!");
			CORE_ASSERT(false);
			return;
		}

		
	}
	void Application::Run()
	{
		while (m_Running)
		{
			m_Renderer->Render();
			m_Window->Update();
		}
	}
	void Application::Close()
	{
		CORE_INFO("Closing application!");
		m_Renderer->Shutdown();
		m_Window->Shutdown();
	}
	void Application::OnResize(i32 width, i32 height)
	{
		m_Renderer->OnResize(width, height);
	}
}


