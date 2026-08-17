#include <pch.h>
#include "Input.h"

#include <Zephyr/Renderer/DeviceManager.h>

#include <GLFW/glfw3.h>

#include "Core/Application.h"

namespace Zephyr
{

	Input::Input(GLFWwindow* windowHandle)
		: m_WindowHandle(windowHandle)
	{
		CORE_ASSERT(m_WindowHandle);
	}
	bool Input::IsKeyDown(KeyCode keycode)
	{
		int state = glfwGetKey(m_WindowHandle, (int)keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		int state = glfwGetMouseButton(m_WindowHandle, (int)button);
		return state == GLFW_PRESS;
	}

	V2 Input::GetMousePosition()
	{
		double x, y;
		glfwGetCursorPos(m_WindowHandle, &x, &y);
		return { (float)x, (float)y };
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		glfwSetInputMode(m_WindowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
	}
}
