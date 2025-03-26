#pragma once
#include <Zephyr/Renderer/GraphicsAPI.h>
#include <Zephyr/Renderer/RenderHardwareInterface.h>
#include <Zephyr/Renderer/Shader.h>
#include <Zephyr/Renderer/RenderDevice.h>
#include <Zephyr/Renderer/Camera.h>
#include <Zephyr/Renderer/Framebuffer.h>

namespace Zephyr
{
	class Renderer final
	{
	public:
		Renderer() = default;
		~Renderer() = default;

		DEFAULT_MOVE_AND_COPY(Renderer)

		bool Initialize(GraphicsAPI api);
		void Shutdown();

		void OnResize(u32 width, u32 height);
		void BeginFrame();
		void EndFrame();
		void ImGuiNewFrame();
		void ImGuiEndFrame();
		void RebuildFontTextures() const;

		NODISCARD const RenderHardwareInterface& GetRenderHardwareInterface() const { return *m_GraphicsInterface; }

		NODISCARD Ref<Framebuffer> GetGBuffer() const { return m_GBuffer; }
		NODISCARD Ref<Framebuffer> GetViewPort() const { return m_ViewPort; }

		NODISCARD Camera& GetMainCamera() { return m_MainCamera; }
	private:
		bool InitImGui();
		void ShutdownImGui();
		

		Scope<RenderHardwareInterface> CreateRHI(GraphicsAPI api);
	protected:
		GraphicsAPI m_API = GraphicsAPI::MAX;
		Scope<RenderHardwareInterface> m_GraphicsInterface = nullptr;
		ShaderLibrary m_Library;

		Ref<Framebuffer> m_GBuffer;
		Ref<Framebuffer> m_ViewPort;
		Camera m_MainCamera;
	};
}