#pragma once

#include <Zephyr/Renderer/Renderer.h>
#include <Zephyr/Renderer/Window.h>

namespace Zephyr
{
	namespace ECS
	{
		class Scene;
	}
	struct ApplicationCommandLineArgs
	{
		int Count = 0;
		char** Args = nullptr;

		const char* operator[](const int index) const
		{
			return Args[index];
		}
	};

	struct ApplicationSpecification
	{
		ApplicationCommandLineArgs Args = {};
		String Name = {};
		Path WorkingDir = {};
		Window::WindowData WindowData = {};
	};

	
	class Application
	{
	public:
		Application(const ApplicationSpecification& specs);
		virtual ~Application() = default;

		void Run();
		void Close();
		void RequestClose() { m_Running = false; }

		virtual void OnResize(u32 width, u32 height) ;

		NODISCARD static Application& Get() { return *s_Instance; }
		NODISCARD const ApplicationSpecification& Specification() const { return m_Specification; }
		NODISCARD virtual Ref<ECS::Scene> GetActiveScene() const = 0;
	protected:
		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnImGuiUpdate() = 0;
		virtual void OnShutdown() = 0;

	private:
		void LoadConfig();
		void SaveConfig();

	private:
		ApplicationSpecification m_Specification;
		bool m_Running = false;



	private:
		static Application* s_Instance;
	};

	Application* CreateApplication(const ApplicationCommandLineArgs& args);
}