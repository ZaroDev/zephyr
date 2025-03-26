#include <pch.h>
#include "Window.h"

#include <GLFW/glfw3.h>
#ifdef PLATFORM_WINDOWS
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#else
#endif

#include <Zephyr/Core/Application.h>

namespace Zephyr
{
	static void glfwErrorCallback(int code, const char* error)
	{
		CORE_ERROR("GLFW error {0}: {1}", code, error);
	}

	
	bool Window::Initialize(const WindowData& data)
	{
		m_WindowData = data;
		CORE_ASSERT(!m_Window, "Window is already initialized");
	
		if (!glfwInit())
		{
			CORE_ERROR("Unable to initialize GLFW!");
			return false;
		}

		glfwSetErrorCallback(glfwErrorCallback);

		glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		

		const String title = std::format("{0} <{1}>", m_WindowData.Title, GetGraphicsName(m_WindowData.API));

		m_Window = glfwCreateWindow(m_WindowData.Width, m_WindowData.Height, title.c_str(), nullptr, nullptr);
		CORE_ASSERT(m_Window, "Failed to create GLFW window!");

		glfwSetWindowUserPointer(m_Window, &m_WindowData);
		glfwSetWindowCloseCallback(m_Window, [](GLFWwindow* window)
			{
				Application::Get().RequestClose();
			});

		glfwSetFramebufferSizeCallback(m_Window, [](GLFWwindow* window, i32 width, i32 height)
			{
				WindowData* data = static_cast<WindowData*>(glfwGetWindowUserPointer(window));
				data->Width = width;
				data->Height = height;

				Application::Get().OnResize(width, height);
			});

		CORE_INFO("Created window {0}:", m_WindowData.Title.c_str());
		CORE_INFO(" - Size: {0}x{1}p", m_WindowData.Width, m_WindowData.Height);
		CORE_INFO(" - Fullscreen: {0}", m_WindowData.Fullscreen);
		CORE_INFO(" - Vsync: {0}", m_WindowData.Vsync);
		return true;
	}

	void Window::Update()
	{
		glfwPollEvents();
		
	}
	void Window::Shutdown()
	{
		CORE_INFO("Closing window!");
		glfwDestroyWindow(m_Window);
		glfwTerminate();

		m_Window = nullptr;
	}

	

	NODISCARD void* Window::GetOSWindowPointer() const
	{
#ifdef PLATFORM_WINDOWS
		return (void*)glfwGetWin32Window(m_Window);
#else
#error "Window implementation not found!"
		return nullptr;
#endif
	}
}
