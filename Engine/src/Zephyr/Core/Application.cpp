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

		if (FileSystem::Exists(m_Specification.WorkingDir))
		{
			FileSystem::WorkingDirectory(m_Specification.WorkingDir);
		}

		Window::Initialize(m_Specification.WindowData);
		Renderer::Initialize(m_Specification.WindowData.API);
	}
	void Application::Run()
	{
		OnInit();

		while (m_Running)
		{
			Window::Update();
			OnUpdate();
			Renderer::BeginFrame();
			Renderer::ImGuiNewFrame();
			OnImGuiUpdate();
			Renderer::ImGuiEndFrame();
			Renderer::EndFrame();
		}

		OnShutdown();
	}
	void Application::Close()
	{
		CORE_INFO("Closing application!");
		Renderer::Shutdown();
		Window::Shutdown();
	}
	void Application::OnResize(i32 width, i32 height)
	{
		Renderer::OnResize(width, height);
	}
	void Application::LoadConfig()
	{
	}
	void Application::SaveConfig()
	{
	}
}


