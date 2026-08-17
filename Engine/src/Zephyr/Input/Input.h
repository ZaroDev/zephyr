/*
MIT License

Copyright (c) 2025 ZaroDev

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
#pragma once

#include "KeyCodes.h"
#include <Zephyr/Modules/IModule.h>
#include <Zephyr/Math/MathTypes.h>


struct GLFWwindow;
namespace Zephyr
{
	class Input final : public IModule
	{
	public:
		Input(GLFWwindow* windowHandle);
		~Input() = default;

		DEFAULT_MOVE_AND_COPY(Input);

		virtual bool Initialize() override { return true; }
		virtual void Shutdown() override{}
		virtual String GetName() const override { return "Input"; }
		virtual int GetPriority() const override { return 0; }
		virtual UpdateFlags GetUpdateFlags() const override { return UpdateFlags::None; }
		virtual bool IsCoreModule() const override { return true; }

		bool IsKeyDown(KeyCode keycode);
		bool IsMouseButtonDown(MouseButton button);
		V2	GetMousePosition();
		void SetCursorMode(CursorMode mode);
	private:
		GLFWwindow* m_WindowHandle = nullptr;
	};
};