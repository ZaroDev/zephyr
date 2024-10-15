#pragma once

#include "D3D11Common.h"
#include <Zephyr/Renderer/RenderHardwareInterface.h>
#include <Zephyr/Renderer/Model.h>
#include <Zephyr/Renderer/Camera.h>

#ifdef PLATFORM_WINDOWS
#include "D3D11Texture.h"

namespace Zephyr::D3D11::Core
{
	struct SceneConstantBuffer
	{
		Mat4 Model;
		Mat4 View;
		Mat4 Projection;
	};
	static_assert(sizeof(SceneConstantBuffer) % 16 == 0, "Constant Buffer size must be 16-byte aligned");

	bool Init();
	bool InitRenderPasses();
	void Shutdown();
	void OnResize(i32 width, i32 height);
	void BeginFrame(Camera& camera);
	void EndFrame();

	void CreateVertexBuffer();
	bool InitImGui();
	void ImGuiNewFrame();
	void ImGuiEndFrame();
	void ShutdownImGui();

	bool CreateSwapChainResources();
	void DestroySwapChainResources();

	NODISCARD ID3D11Device& Device();
	NODISCARD ID3D11DeviceContext& DeviceContext();
	NODISCARD RenderDevice GetRenderDevice();
}

#endif