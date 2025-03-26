#pragma once
#include <Zephyr/Math/MathTypes.h>
#include <Zephyr/Renderer/RenderDevice.h>

namespace Zephyr
{

	class RenderHardwareInterface
	{
	public:
		virtual ~RenderHardwareInterface() = default;
		virtual bool Init() = 0;
		virtual void Shutdown() = 0;

		virtual NODISCARD const char* GetName() const = 0;
		virtual NODISCARD RenderDevice GetRenderDevice() const = 0;
		virtual void OnResize(u32 width, u32 height) = 0;

		virtual void BeginFrame() = 0;
		virtual void EndFrame() = 0;

		virtual bool ImGuiInit() { return false;  }
		virtual void ImGuiNewFrame() {}
		virtual void ImGuiEndFrame() {}
		virtual void ImGuiShutdown() {}
		virtual void RebuildFontTextures() const {}
	};
}