#include <pch.h>
#include "Application.h"

#include <Time/Time.h>

#include <Window/Window.h>
#include <Input/Input.h>
#include <Renderer/Renderer.h>

namespace Zephyr
{
	// Command-line backend override: --api=<name> or --api <name>, where <name> is one of
	// d3d11/dx11, d3d12/dx12, vulkan/vk (case-insensitive). Falls back to the hardcoded default
	// below if absent or unrecognized - lets the same build switch GraphicsAPI without
	// recompiling (e.g. `Editor.exe --api=vulkan`).
	static GraphicsAPI ResolveGraphicsAPI(const ApplicationCommandLineArgs& args)
	{
		constexpr GraphicsAPI defaultApi = GraphicsAPI::D3D12;

		for (int i = 1; i < args.Count; i++)
		{
			StrView arg = args[i];

			StrView value;
			if (arg.starts_with("--api="))
				value = arg.substr(6);
			else if (arg == "--api" && i + 1 < args.Count)
				value = args[++i];
			else
				continue;

			GraphicsAPI api;
			if (TryParseGraphicsAPI(value, api))
				return api;

			CORE_WARN("Unrecognized --api value '{}', falling back to {}", value, GetGraphicsName(defaultApi));
			return defaultApi;
		}

		return defaultApi;
	}

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

		const GraphicsAPI api = ResolveGraphicsAPI(specs.Args);
		CORE_INFO("Using graphics backend: {}", GetGraphicsName(api));

		RendererData renderData
		{
			.Api = api,
			.Window = window,
		};

		auto renderer = m_ModuleManager.RegisterModule<Renderer>(renderData);

		// OnImGui() can't be called from Application::Render() below - by the time
		// m_ModuleManager.Render() returns, this Renderer has already called ImGui::Render() for
		// the frame, and further ImGui::Begin()/End() calls would hit ImGui's "Forgot to call
		// NewFrame()?" assert. Renderer invokes this at the correct point instead (see
		// Renderer::SetImGuiCallback).
		renderer->SetImGuiCallback([this](float deltaTime) { OnImGui(deltaTime); });

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
		// OnImGui(deltaTime) is invoked from within Renderer::Render() (called above via
		// m_ModuleManager.Render()), not here - see the SetImGuiCallback wiring in the constructor.
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


