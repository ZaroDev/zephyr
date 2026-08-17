#include <pch.h>
#include "Window.h"

#include <GLFW/glfw3.h>
#include <Zephyr/Core/Application.h>

namespace Zephyr
{
	Window::Window(const WindowParams& params)
		: m_Params(params)
	{
	}
	bool Window::Initialize()
	{
		if (!glfwInit())
		{
			CORE_ERROR("Failed to initialize GLFW!");
			return false;
		}

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

		m_WindowHandle = glfwCreateWindow(m_Params.Width, m_Params.Height, m_Params.Title.c_str(), nullptr, nullptr);
		CORE_ASSERT(m_WindowHandle, "Failed to create GLFW window");

		glfwSetWindowUserPointer(m_WindowHandle, &m_Params);
		glfwSetWindowCloseCallback(m_WindowHandle, [](GLFWwindow* window)
			{
				Application::Get().RequestClose();
			});

		CORE_INFO("Created window {0}:", m_Params.Title);
		CORE_INFO(" - Size: {0}x{1}p", m_Params.Width, m_Params.Height);
		CORE_INFO(" - Fullscreen: {0}", m_Params.Fullscreen);
		CORE_INFO(" - Vsync: {0}", m_Params.Vsync);

		return true;
	}
	void Window::Shutdown()
	{
		glfwDestroyWindow(m_WindowHandle);
		glfwTerminate();
	}
	bool Window::IsOpen() const
	{
		return !glfwWindowShouldClose(m_WindowHandle);
	}
	void Window::PostRenderer(float deltaTime)
	{
		glfwPollEvents();
	}
}
