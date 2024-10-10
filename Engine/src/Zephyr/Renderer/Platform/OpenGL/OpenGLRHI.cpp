#include <pch.h>
#include "OpenGLRHI.h"

#include "OpenGLRenderer.h"
#include <Zephyr/Renderer/RenderHardwareInterface.h>

namespace Zephyr::OpenGL
{
	void GetPlatformInterface(RenderHardwareInterface& rhi)
	{
		rhi.Core.Init = Core::Init;
		rhi.Core.BeginFrame = Core::BeginFrame;
		rhi.Core.EndFrame = Core::EndFrame;
		rhi.Core.OnResize = Core::OnResize;
		rhi.Core.Shutdown = Core::Shutdown;

		rhi.ImGui.Init = Core::InitImGui;
		rhi.ImGui.NewFrame = Core::ImGuiNewFrame;
		rhi.ImGui.EndFrame = Core::ImGuiEndFrame;
		rhi.ImGui.Shutdown = Core::ShutdownImGui;
	}

}