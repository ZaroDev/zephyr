#pragma once

#include <Zephyr/Renderer/Renderer.h>
#include <Zephyr/Renderer/Window.h>

namespace Zephyr
{
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
		GraphicsAPI API = {};
		std::string Name = {};
		Path WorkingDir = {};
	};


	class Application
	{
	public:
		Application(const ApplicationSpecification& specs);
		virtual ~Application() = default;

		void Run();
		void Close();
		void RequestClose() { m_Running = false; }

		void OnResize(i32 width, i32 height);

		NODISCARD static Application& Get() { return *s_Instance; }
		
		NODISCARD Window& GetWindow() const { return *m_Window; }
		NODISCARD Renderer& GetRenderer() const { return *m_Renderer;  }

		NODISCARD const ApplicationSpecification& Specification() const { return m_Specification; }
	
	private:
		void LoadConfig();
		void SaveConfig();

	private:
		ApplicationSpecification m_Specification;
		bool m_Running = false;

		Scope<Renderer> m_Renderer;
		Scope<Window> m_Window;

	private:
		static Application* s_Instance;
	};

	Application* CreateApplication(const ApplicationCommandLineArgs& args);
}