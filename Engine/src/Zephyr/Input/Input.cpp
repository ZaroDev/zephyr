/*
MIT License

Copyright (c) 2023 Víctor Falcón Zaro

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <pch.h>
#include "Input.h"

#include <Zephyr/Renderer/Window.h>

#include <GLFW/glfw3.h>

namespace Zephyr::Input
{
	bool Input::IsKeyDown(KeyCode keycode)
	{
		GLFWwindow* windowHandle = Window::GetGLFWWindow();
		int state = glfwGetKey(windowHandle, (int)keycode);
		return state == GLFW_PRESS || state == GLFW_REPEAT;
	}

	bool Input::IsMouseButtonDown(MouseButton button)
	{
		GLFWwindow* windowHandle = Window::GetGLFWWindow();
		int state = glfwGetMouseButton(windowHandle, (int)button);
		return state == GLFW_PRESS;
	}

	V2 Input::GetMousePosition()
	{
		GLFWwindow* windowHandle = Window::GetGLFWWindow();

		double x, y;
		glfwGetCursorPos(windowHandle, &x, &y);
		return { (float)x, (float)y };
	}

	void Input::SetCursorMode(CursorMode mode)
	{
		GLFWwindow* windowHandle = Window::GetGLFWWindow();
		glfwSetInputMode(windowHandle, GLFW_CURSOR, GLFW_CURSOR_NORMAL + (int)mode);
	}
}