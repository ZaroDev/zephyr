#pragma once

#include "OpenGLCommon.h"
#include <Zephyr/Renderer/RenderHardwareInterface.h>

namespace Zephyr::OpenGL::Core
{
	bool Init();
	void Shutdown();
	void OnResize(i32 width, i32 height);
	void BeginFrame(Camera& camera);
	void EndFrame();
	//void CreateTexture(D3D11Texture2D& texture, Buffer buffer = Buffer());


	bool InitImGui();
	void ImGuiNewFrame();
	void ImGuiEndFrame();
	void ShutdownImGui();

	void CreateVertexBuffer();


	NODISCARD RenderDevice GetRenderDevice();
}