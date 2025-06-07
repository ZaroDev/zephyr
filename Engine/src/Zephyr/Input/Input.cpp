#include <pch.h>
#include "Input.h"

#include <Zephyr/Renderer/DeviceManager.h>

#include <GLFW/glfw3.h>

#include "Core/Application.h"

namespace Zephyr::Input
{
	bool Input::IsKeyDown(KeyCode keycode)
	{
		GLFWwindow* windowHandle = Application::Get().GetDeviceManager().GetWindow();
		int state = glfwGetKey(windowHandle, (int)keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		GLFWwindow* windowHandle = Application::Get().GetDeviceManager().GetWindow();
		int state = glfwGetMouseButton(windowHandle, (int)button);
		return state == GLFW_PRESS;
	}

	V2 Input::GetMousePosition()
	{
		GLFWwindow* windowHandle = Application::Get().GetDeviceManager().GetWindow();

		double x, y;
		glfwGetCursorPos(windowHandle, &x, &y);
		return { (float)x, (float)y };
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		GLFWwindow* windowHandle = Application::Get().GetDeviceManager().GetWindow();
		glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
	}
}
