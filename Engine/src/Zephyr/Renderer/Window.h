#pragma once
#include <Zephyr/Renderer/GraphicsAPI.h>

struct GLFWwindow;

namespace Zephyr
{
	class Window final
	{
	public:
		struct WindowData
		{
			u32 Width = 1280;
			u32 Height = 720;
			String Title = "Zephyr";
			bool Fullscreen = false;
			bool Vsync = false;
			GraphicsAPI API;
		};

		bool Initialize(const WindowData& data);
		void Update();
		void Shutdown();
		NODISCARD GLFWwindow* GetGLFWWindow() const { return m_Window; }
		NODISCARD void* GetOSWindowPointer() const;
		NODISCARD const WindowData& GetWindowData() const { return m_WindowData; }

	private:
		WindowData m_WindowData = {};
		GLFWwindow* m_Window = nullptr;
	};
}