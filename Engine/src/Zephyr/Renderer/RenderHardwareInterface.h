#pragma once
#include <Zephyr/Math/MathTypes.h>
#include <Zephyr/Renderer/Window.h>
#include <Zephyr/Renderer/RenderDevice.h>

namespace Zephyr
{
	class Model;
	struct RenderHardwareInterface final
	{
		struct
		{
			bool (*Init)(void);
			RenderDevice (*GetRenderDevice)(void);
			void (*Shutdown)();
			void (*OnResize)(i32 width, i32 height);
			void (*BeginFrame)();
			void (*EndFrame)();
		} Core;

		struct
		{
			bool (*Init)();
			void (*NewFrame)();
			void (*EndFrame)();
			void (*Shutdown)();
		} ImGui;
		
	};
}