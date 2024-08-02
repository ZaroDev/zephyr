#pragma once
#include <Zephyr/Renderer/GraphicsAPI.h>

struct GLFWwindow;

namespace Zephyr
{
	struct WindowData
	{
		i32 Width = 1280;
		i32 Height = 720;
		std::string Title = "Zephyr";
		bool Fullscreen = false;
		bool Vsync = false;
		GraphicsAPI API;
	};

	class Window final
	{
	public:
		Window(const WindowData& data);
		~Window() = default;

		bool Init();
		void Update();
		void Shutdown();

		NODISCARD const WindowData& Data() const { return m_Data; }
		NODISCARD void* OSWindow() const;
	private:
		WindowData m_Data;
		GLFWwindow* m_InternalWindow = nullptr;
	};
}