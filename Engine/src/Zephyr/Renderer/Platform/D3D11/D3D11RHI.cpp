#include <pch.h>
#include "D3D11RHI.h"

#include "D3D11Renderer.h"
#include <Zephyr/Renderer/RenderHardwareInterface.h>

namespace Zephyr::D3D11
{
	void GetPlatformInterface(RenderHardwareInterface& rhi)
	{
		rhi.Core.Init = Core::Init;
		rhi.Core.GetRenderDevice = Core::GetRenderDevice;
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
