#pragma once

#include "D3D11Common.h"
#include <Zephyr/Renderer/RenderHardwareInterface.h>
#include <Zephyr/Renderer/Model.h>

#ifdef PLATFORM_WINDOWS
#include "D3D11Texture.h"

namespace Zephyr::D3D11::Core
{
	struct VertexPositionColor
	{
		V3 Position;
		Color3 Color;
	};


	bool Init();
	void Shutdown();
	void OnResize(i32 width, i32 height);
	void BeginFrame();
	void EndFrame();
	void CreateTexture(D3D11Texture2D& texture, Buffer buffer = Buffer());

	void CreateVertexBuffer();
	bool InitImGui();
	void ImGuiNewFrame();
	void ImGuiEndFrame();
	void ShutdownImGui();

	bool CreateSwapChainResources();
	void DestroySwapChainResources();

	NODISCARD ID3D11Device& Device();
	NODISCARD ID3D11DeviceContext& DeviceContext();
}

#endif