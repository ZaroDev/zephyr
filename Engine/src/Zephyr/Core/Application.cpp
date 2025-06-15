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

		
		Project::New();

		m_DeviceManager = DeviceManager::Create(specs.GraphicsBackend);
		m_DeviceManager->CreateWindowDeviceAndSwapChain(specs.DeviceParams, specs.Name.c_str());
	}
	void Application::Run()
	{
		OnInit();
		m_DeviceManager->Update();
		OnShutdown();
	}
	void Application::Close()
	{
		CORE_INFO("Closing application!");
		m_DeviceManager->Shutdown();
		delete m_DeviceManager;
	}
	void Application::OnResize(u32 width, u32 height)
	{
	}
	void Application::LoadConfig()
	{
	}
	void Application::SaveConfig()
	{
	}
}


