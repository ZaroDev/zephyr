#include <pch.h>
#include "Renderer.h"

#include <imgui.h>

#include <Zephyr/Renderer/Platform/D3D11/D3D11RHI.h>
#include <Zephyr/Renderer/Platform/OpenGL/OpenGLRHI.h>

namespace Zephyr::Renderer
{
	namespace
	{
		GraphicsAPI g_API = GraphicsAPI::MAX;
		RenderHardwareInterface g_GraphicsInterface{};
		ShaderLibrary g_Library;
		RenderDevice g_Device;

		bool SetPlatformInterface(GraphicsAPI api)
		{
			switch (api)
			{
			case GraphicsAPI::OPENGL: OpenGL::GetPlatformInterface(g_GraphicsInterface); return true;
			case GraphicsAPI::DX11: D3D11::GetPlatformInterface(g_GraphicsInterface); return true;
			}

			CORE_ASSERT(false);

			return false;
		}
	}


	bool Initialize(GraphicsAPI api)
	{
		g_API = api;

		bool ret = true;
		ret &= SetPlatformInterface(api);
		ret &= g_GraphicsInterface.Core.Init();
		ret &= g_Library.LoadEngineShaders();

		ret &= InitImGui();
		ret &= g_GraphicsInterface.ImGui.Init();


		return ret;
	}

	void Shutdown()
	{
		CORE_INFO("Closing renderer!");
		g_GraphicsInterface.ImGui.Shutdown();
		ShutdownImGui();
		g_GraphicsInterface.Core.Shutdown();
	}
	void OnResize(i32 width, i32 height)
	{
		CORE_INFO("Resize event: {0}x{1}p", width, height);
		g_GraphicsInterface.Core.OnResize(width, height);
	}
	void BeginFrame()
	{
		g_GraphicsInterface.Core.BeginFrame();
	}
	void EndFrame()
	{
		g_GraphicsInterface.Core.EndFrame();
	}
	void ImGuiNewFrame()
	{
		g_GraphicsInterface.ImGui.NewFrame();
		ImGui::NewFrame();
	}
	void ImGuiEndFrame()
	{
		ImGui::Render();
		g_GraphicsInterface.ImGui.EndFrame();
	}

	RenderHardwareInterface& GetHardwareInterface()
	{
		return g_GraphicsInterface;
	}

	ShaderLibrary& GetShaderLibrary()
	{
		return g_Library;
	}

	GraphicsAPI GetAPI()
	{
		return g_API;
	}

	RenderDevice GetRenderDevice()
	{
		return g_Device;
	}

	bool InitImGui()
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();

		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_IsSRGB;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows

		ImGui::StyleColorsDark();

		return true;
	}
	void ShutdownImGui()
	{
		ImGui::DestroyContext();
	}
}