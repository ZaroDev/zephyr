#pragma once
#include <Zephyr/Renderer/GraphicsAPI.h>
#include <Zephyr/Renderer/RenderHardwareInterface.h>
#include <Zephyr/Renderer/Shader.h>
#include <Zephyr/Renderer/RenderDevice.h>

#include <Zephyr/Renderer/Camera.h>

namespace Zephyr::Renderer
{
	enum class RenderingPath
	{
		FORWARD,
		DEFERRED,

		MAX
	};


	bool Initialize(GraphicsAPI api, RenderingPath renderPath = RenderingPath::DEFERRED);
	void Shutdown();

	void OnResize(i32 width, i32 height);
	void BeginFrame();
	void EndFrame();

	void ImGuiNewFrame();
	void ImGuiEndFrame();

	NODISCARD RenderHardwareInterface& GetHardwareInterface();
	NODISCARD ShaderLibrary& GetShaderLibrary();
	NODISCARD GraphicsAPI GetAPI();
	NODISCARD RenderDevice GetRenderDevice();
	NODISCARD RenderingPath GetRenderPath();

	bool InitImGui();
	void ShutdownImGui();

	Camera& GetMainCamera();

}