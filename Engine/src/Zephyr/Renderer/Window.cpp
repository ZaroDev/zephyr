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

	Window::Window(const WindowData& data)
	{
		m_Data = data;
	}
	bool Window::Init()
	{
		CORE_ASSERT(!m_InternalWindow, "Window is already initialized");
	
		if (!glfwInit())
		{
			CORE_ERROR("Unable to initialize GLFW!");
			return false;
		}

		glfwSetErrorCallback(glfwErrorCallback);

		glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_FALSE);
		if (m_Data.API != GraphicsAPI::OPENGL)
		{
			glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		}

		const std::string title = std::format("{0} <{1}>", m_Data.Title, GetGraphicsName(m_Data.API));

		m_InternalWindow = glfwCreateWindow(m_Data.Width, m_Data.Height, title.c_str(), nullptr, nullptr);
		CORE_ASSERT(m_InternalWindow, "Failed to create GLFW window!");

		glfwSetWindowUserPointer(m_InternalWindow, &m_Data);
		glfwSetWindowCloseCallback(m_InternalWindow, [](GLFWwindow* window)
			{
				Application::Get().RequestClose();
			});

		glfwSetFramebufferSizeCallback(m_InternalWindow, [](GLFWwindow* window, i32 width, i32 height)
			{
				WindowData* data = reinterpret_cast<WindowData*>(glfwGetWindowUserPointer(window));
				data->Width = width;
				data->Height = height;

				Application::Get().OnResize(width, height);
			});

		CORE_INFO("Created window {0}:", m_Data.Title.c_str());
		CORE_INFO(" - Size: {0}x{1}p", m_Data.Width, m_Data.Height);
		CORE_INFO(" - Fullscreen: {0}", m_Data.Fullscreen);
		CORE_INFO(" - Vsync: {0}", m_Data.Vsync);
		return true;
	}
	void Window::Update()
	{
		glfwPollEvents();
	}
	void Window::Shutdown()
	{
		CORE_INFO("Closing window!");
		glfwDestroyWindow(m_InternalWindow);
		glfwTerminate();

		m_InternalWindow = nullptr;
	}
	NODISCARD void* Window::OSWindow() const
	{
#ifdef PLATFORM_WINDOWS
		return (void*)glfwGetWin32Window(m_InternalWindow);
#else
#error "Window implementation not found!"
		return nullptr;
#endif
	}
}