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

		DEFAULT_MOVE_AND_COPY(Application)

		void Run();
		void Close();
		void RequestClose() { m_Running = false; }

		virtual void OnResize(u32 width, u32 height) ;

		NODISCARD static Application& Get() { return *s_Instance; }
		NODISCARD const ApplicationSpecification& Specification() const { return m_Specification; }
		NODISCARD virtual Ref<ECS::Scene> GetActiveScene() const = 0;

		NODISCARD const Window& GetWindow() const { return m_Window; }
		NODISCARD Renderer& GetRenderer() { return m_Renderer; }
	protected:
		virtual void OnInit() = 0;
		virtual void OnUpdate() = 0;
		virtual void OnImGuiUpdate() = 0;
		virtual void OnShutdown() = 0;

		void LoadConfig();
		void SaveConfig();

		ApplicationSpecification m_Specification;
		bool m_Running = false;

		Renderer m_Renderer{};
		Window m_Window{};

		static Application* s_Instance;
	};

	Application* CreateApplication(const ApplicationCommandLineArgs& args);
}