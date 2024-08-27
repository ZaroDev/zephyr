#pragma once
#include <Zephyr/Renderer/GraphicsAPI.h>
#include <Zephyr/Renderer/RenderHardwareInterface.h>
#include <Zephyr/Renderer/Shader.h>

namespace Zephyr::Renderer
{
	struct RenderDevice
	{
		String Name;
		String Vendor;
	};

	bool Initialize(GraphicsAPI api);
	void Shutdown();

	void OnResize(i32 width, i32 height);
	void BeginFrame();
	void EndFrame();

	void ImGuiNewFrame();
	void ImGuiEndFrame();

	RenderHardwareInterface& GetHardwareInterface();
	ShaderLibrary& GetShaderLibrary();
	GraphicsAPI GetAPI();
	RenderDevice GetRenderDevice();

	bool InitImGui();
	void ShutdownImGui();



}