#pragma once
#include <Zephyr/Renderer/GraphicsAPI.h>

struct GLFWwindow;

namespace Zephyr::Window
{
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
	NODISCARD GLFWwindow* GetGLFWWindow();
	NODISCARD void* GetOSWindowPointer();
	NODISCARD const WindowData& GetWindowData();

}