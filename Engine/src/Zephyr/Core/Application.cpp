#include <pch.h>
#include "Application.h"

#include <Time/Time.h>

#include <Window/Window.h>
#include <Input/Input.h>
#include <Renderer/Renderer.h>

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
		
		WindowParams params
		{
			.Title = specs.Name,
			.Width = 1920,
			.Height = 1080,
		};

		auto window = m_ModuleManager.RegisterModule<Window>(params);
		m_ModuleManager.RegisterModule<Input>(window->GetGLFWHandle());

		RendererData renderData
		{
			.Api = GraphicsAPI::VULKAN,
			.Window = window,
		};

		m_ModuleManager.RegisterModule<Renderer>(renderData);

		OnInit();
	}
	void Application::Run()
	{
		while (m_Running)
		{
			Time::StartTimeUpdate();
			float deltaTime = Time::GetDeltaTime();

			m_ModuleManager.SortModules();

			PreUpdate(deltaTime);
			Update(deltaTime);
			PostUpdate(deltaTime);


			PreRender(deltaTime);
			Render(deltaTime);
			PostRenderer(deltaTime);


			Time::EndTimeUpdate();
		}
	}

	void Application::PreUpdate(float deltaTime)
	{
		m_ModuleManager.PreUpdate(deltaTime);
		OnPreUpdate(deltaTime);
	}

	void Application::Update(float deltaTime)
	{
		m_ModuleManager.Update(deltaTime);
		OnUpdate(deltaTime);
	}

	void Application::PostUpdate(float deltaTime)
	{
		m_ModuleManager.PostUpdate(deltaTime);
		OnPostUpdate(deltaTime);
	}

	void Application::PrePhysics(float deltaTime)
	{
		m_ModuleManager.PrePhysics(deltaTime);
		OnPrePhysicsUpdate(deltaTime);
	}

	void Application::Physics(float deltaTime)
	{
		m_ModuleManager.Physics(deltaTime);
		OnPhysicsUpdate(deltaTime);
	}

	void Application::PostPhysics(float deltaTime)
	{
		m_ModuleManager.PostPhysics(deltaTime);
		OnPostPhysicsUpdate(deltaTime);
	}

	void Application::PreRender(float deltaTime)
	{
		m_ModuleManager.PreRender(deltaTime);
		OnPreRender(deltaTime);
	}

	void Application::Render(float deltaTime)
	{
		m_ModuleManager.Render(deltaTime);
		OnRender(deltaTime);
		//OnImGui(deltaTime);
	}
	
	void Application::PostRenderer(float deltaTime)
	{
		m_ModuleManager.PostRenderer(deltaTime);
		OnPostRenderer(deltaTime);
	}

	void Application::Close()
	{
		CORE_INFO("Closing application!");
		m_ModuleManager.Shutdown();
	}
}


