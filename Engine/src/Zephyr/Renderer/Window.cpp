#include <pch.h>
#include "Window.h"

#include <GLFW/glfw3.h>
#ifdef PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#else
#endif

#include <Zephyr/Core/Application.h>

namespace Zephyr::Window
{
	static void glfwErrorCallback(int code, const char* error)
	{
		CORE_ERROR("GLFW error {0}: {1}", code, error);
	}

	namespace 
	{
		WindowData g_WindowData;
		GLFWwindow* g_Window = nullptr;
	}

	
	bool Initialize(const WindowData& data)
	{
		g_WindowData = data;
		CORE_ASSERT(!g_Window, "Window is already initialized");
	
		if (!glfwInit())
		{
			CORE_ERROR("Unable to initialize GLFW!");
			return false;
		}

		glfwSetErrorCallback(glfwErrorCallback);

		glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
		if (g_WindowData.API != GraphicsAPI::OPENGL)
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		}

		const String title = std::format("{0} <{1}>", g_WindowData.Title, GetGraphicsName(g_WindowData.API));

		g_Window = glfwCreateWindow(g_WindowData.Width, g_WindowData.Height, title.c_str(), nullptr, nullptr);
		CORE_ASSERT(g_Window, "Failed to create GLFW window!");

		glfwSetWindowUserPointer(g_Window, &g_WindowData);
		glfwSetWindowCloseCallback(g_Window, [](GLFWwindow* window)
			{
				Application::Get().RequestClose();
			});

		glfwSetFramebufferSizeCallback(g_Window, [](GLFWwindow* window, i32 width, i32 height)
			{
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
				data->Width = width;
				data->Height = height;

				Application::Get().OnResize(width, height);
			});

		CORE_INFO("Created window {0}:", g_WindowData.Title.c_str());
		CORE_INFO(" - Size: {0}x{1}p", g_WindowData.Width, g_WindowData.Height);
		CORE_INFO(" - Fullscreen: {0}", g_WindowData.Fullscreen);
		CORE_INFO(" - Vsync: {0}", g_WindowData.Vsync);
		return true;
	}



	void Update()
	{
		glfwPollEvents();
	}
	void Shutdown()
	{
		CORE_INFO("Closing window!");
		glfwDestroyWindow(g_Window);
		glfwTerminate();

		g_Window = nullptr;
	}

	GLFWwindow* GetGLFWWindow()
	{
		return g_Window;
	}


	NODISCARD void* GetOSWindowPointer() 
	{
#ifdef PLATFORM_WINDOWS
		return (void*)glfwGetWin32Window(g_Window);
#else
#error "Window implementation not found!"
		return nullptr;
#endif
	}

	const WindowData& GetWindowData()
	{
		return g_WindowData;
	}
}
