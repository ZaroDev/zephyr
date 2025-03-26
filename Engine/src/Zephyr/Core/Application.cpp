#include <pch.h>
#include "Application.h"

#include <Time/Time.h>

#include "Project/Project.h"

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
		Project::New();
		m_Window.Initialize(m_Specification.WindowData);
		m_Renderer.Initialize(m_Specification.WindowData.API);
	}
	void Application::Run()
	{
		OnInit();

		while (m_Running)
		{
			Time::StartTimeUpdate();

			m_Window.Update();
			OnUpdate();
			m_Renderer.ImGuiNewFrame();
			OnImGuiUpdate();
			m_Renderer.BeginFrame();
			m_Renderer.ImGuiEndFrame();
			m_Renderer.EndFrame();

			Time::EndTimeUpdate();
		}

		OnShutdown();
	}
	void Application::Close()
	{
		CORE_INFO("Closing application!");
		m_Renderer.Shutdown();
		m_Window.Shutdown();
	}
	void Application::OnResize(u32 width, u32 height)
	{
		m_Renderer.OnResize(width, height);
	}
	void Application::LoadConfig()
	{
	}
	void Application::SaveConfig()
	{
	}
}


